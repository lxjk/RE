#pragma once

#include <emmintrin.h>


typedef __m128	Vec128;

#define VecZero()				_mm_setzero_ps()

#define VecSet1(f)				_mm_set1_ps(f)
#define VecSet1_i(i)			_mm_castsi128_ps(_mm_set1_epi32(i))

#define VecSet(x,y,z,w)			_mm_setr_ps(x,y,z,w);
#define VecSet_i(x,y,z,w)		_mm_castsi128_ps(_mm_setr_epi32(x,y,z,w))

namespace Vec128Const {

	const Vec128 VecMaskXYZ		= VecSet_i(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000);
	const Vec128 VecMaskXY		= VecSet_i(0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000);
	const Vec128 VecMaskZW		= VecSet_i(0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF);
	const Vec128 VecMaskW		= VecSet_i(0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF);

	const Vec128 SignMask		= VecSet1(-0.f);
};

#define VecLoad1(f_ptr)			_mm_load1_ps(f_ptr)

#define VecAnd(vec1, vec2)		_mm_and_ps(vec1, vec2)
#define VecOr(vec1, vec2)		_mm_or_ps(vec1, vec2)

#define VecAdd(vec1, vec2)		_mm_add_ps(vec1, vec2)
#define VecSub(vec1, vec2)		_mm_sub_ps(vec1, vec2)
#define VecMul(vec1, vec2)		_mm_mul_ps(vec1, vec2)
#define VecDiv(vec1, vec2)		_mm_div_ps(vec1, vec2)

//#define VecNegate(vec)			_mm_sub_ps(_mm_setzero_ps(), vec)
#define VecNegate(vec)			_mm_xor_ps(vec, Vec128Const::SignMask)

#define VecSqrt(vec)			_mm_sqrt_ps(vec)

#define VecMoveMask(vec)		_mm_movemask_ps(vec)
#define VecCmpLT(vec1, vec2)	_mm_cmplt_ps(vec1, vec2)
#define VecCmpLE(vec1, vec2)	_mm_cmple_ps(vec1, vec2)
#define VecCmpGT(vec1, vec2)	_mm_cmpgt_ps(vec1, vec2)
#define VecCmpGE(vec1, vec2)	_mm_cmpge_ps(vec1, vec2)

// mask is Vec128, only highest bit of each f32 component is used
#define VecBlend(vec1, vec2, mask)			_mm_blendv_ps(vec1, vec2, mask);
// mask is const int, only lowest 4 bits are used
#define VecBlendConst(vec1, vec2, mask)		_mm_blend_ps(vec1, vec2, mask);

#define MakeShuffleMask(x,y,z,w)			(x | (y<<2) | (z<<4) | (w<<6))

// vec(0, 1, 2, 3) -> (vec[x], vec[y], vec[z], vec[w])
#define VecSwizzle(vec, x, y, z, w)			_mm_shuffle_ps(vec, vec, MakeShuffleMask(x,y,z,w))
#define VecSwizzleMask(vec, mask)			_mm_shuffle_ps(vec, vec, mask)

//			0		1		 2		  3
// return (vec1[x], vec1[y], vec2[z], vec2[w])
#define VecShuffle(vec1, vec2, x, y, z, w)	_mm_shuffle_ps(vec1, vec2, MakeShuffleMask(x,y,z,w))
#define VecShuffleMask(vec1, vec2, mask)	_mm_shuffle_ps(vec1, vec2, mask)

// return float value x1*x2 + y1*y2 + z1*z2 + w1*w2
//#define VecDot(vec1, vec2)		_mm_cvtss_f32(_mm_dp_ps(vec1, vec2, 0xF1))
// This is faster than _mm_dp_ps ?!!!
inline float VecDot(Vec128 vec1, Vec128 vec2)
{
	Vec128 t0 = _mm_mul_ps(vec1, vec2);
	Vec128 t1 = _mm_hadd_ps(t0, t0);
	Vec128 t2 = _mm_hadd_ps(t1, t1);
	return _mm_cvtss_f32(t2);
}

inline Vec128 VecDotV(Vec128 vec1, Vec128 vec2)
{
	Vec128 t0 = _mm_mul_ps(vec1, vec2);
	Vec128 t1 = _mm_hadd_ps(t0, t0);
	return _mm_hadd_ps(t1, t1);
}

// return float value x1*x2 + y1*y2 + z1*z2
//#define VecDot3(vec1, vec2)		_mm_cvtss_f32(_mm_dp_ps(vec1, vec2, 0x71))
inline float VecDot3(Vec128 vec1, Vec128 vec2)
{
	Vec128 t0 = _mm_mul_ps(vec1, vec2); // 3, 2, 1, 0
	Vec128 t1 = _mm_hadd_ps(t0, t0); // 3+2, 1+0, 3+2, 1+0
	Vec128 t2 = _mm_movehl_ps(t0, t0); // 3, 2, 3, 2
	Vec128 t3 = _mm_add_ps(t1, t2); // x, x, x, 2+1+0
	return _mm_cvtss_f32(t3);
}
inline Vec128 VecDot3V(Vec128 vec1, Vec128 vec2)
{
	Vec128 t0 = _mm_mul_ps(vec1, vec2); // 3, 2, 1, 0
	Vec128 t1 = _mm_hadd_ps(t0, t0); // 3+2, 1+0, 3+2, 1+0
	Vec128 t2 = _mm_add_ps(t1, t0); // x, 2+1+0, x, x
	return VecSwizzle(t2, 2, 2, 2, 2);
}
// return float value x1*x2 + y1*y2
//#define VecDot2(vec1, vec2)		_mm_cvtss_f32(_mm_dp_ps(vec1, vec2, 0x31))
inline float VecDot2(Vec128 vec1, Vec128 vec2)
{
	Vec128 t0 = _mm_mul_ps(vec1, vec2);
	Vec128 t1 = _mm_hadd_ps(t0, t0);
	return _mm_cvtss_f32(t1);
}
inline Vec128 VecDot2V(Vec128 vec1, Vec128 vec2)
{
	Vec128 t0 = _mm_mul_ps(vec1, vec2);
	Vec128 t1 = _mm_hadd_ps(t0, t0); // 3+2, 1+0, 3+2, 1+0
	return _mm_moveldup_ps(t1); // duplicate 0, 2 to 1, 3
}

// return vector value (x1*x2 + y1*y2, z1*z2 + w1*w2, ?, ?)
//#define VecDot2P(vec1, vec2)	_mm_add_ps(_mm_dp_ps(vec1, vec2, 0x31), _mm_dp_ps(vec1, vec2, 0xC2))
inline Vec128 VecDot2P(Vec128 vec1, Vec128 vec2)
{
	Vec128 v = _mm_mul_ps(vec1, vec2);
	return _mm_hadd_ps(v, v);
}

inline Vec128 VecCross(Vec128 vec1, Vec128 vec2)
{
	// 3 shuffle version: http://threadlocalmutex.com/?p=8
	const int mask = MakeShuffleMask(1, 2, 0, 3);
	Vec128 v1_yzx = VecSwizzleMask(vec1, mask);
	Vec128 v2_yzx = VecSwizzleMask(vec2, mask);
	Vec128 r_zxy = _mm_sub_ps(_mm_mul_ps(vec1, v2_yzx), _mm_mul_ps(vec2, v1_yzx));
	return VecSwizzleMask(r_zxy, mask);
#if 0 // 4 shuffle version
	Vec128 v1_yzx = VecSwizzle(vec1, 1, 2, 0, 3);
	Vec128 v1_zxy = VecSwizzle(vec1, 2, 0, 1, 3);
	Vec128 v2_yzx = VecSwizzle(vec2, 1, 2, 0, 3);
	Vec128 v2_zxy = VecSwizzle(vec2, 2, 0, 1, 3);
	return _mm_sub_ps(_mm_mul_ps(v1_yzx, v2_zxy), _mm_mul_ps(v1_zxy, v2_zxy));
#endif
}