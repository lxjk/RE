#pragma once

#include <emmintrin.h>
#include <smmintrin.h>


typedef __m128	Vec128;

#define VecZero()				_mm_setzero_ps()

#define VecSet1(f)				_mm_set1_ps(f)
#define VecSet1_i(i)			_mm_castsi128_ps(_mm_set1_epi32(i))

#define VecSet(x,y,z,w)			_mm_setr_ps(x,y,z,w)
#define VecSet_i(x,y,z,w)		_mm_castsi128_ps(_mm_setr_epi32(x,y,z,w))

namespace VecConst {

	const Vec128 VecMaskXYZ				= VecSet_i(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000);
	const Vec128 VecMaskXY				= VecSet_i(0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000);
	const Vec128 VecMaskZW				= VecSet_i(0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF);
	const Vec128 VecMaskW				= VecSet_i(0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF);

	const Vec128 SignMask				= VecSet1_i(0x80000000);
	const Vec128 InvSignMask			= VecSet1_i(0x7FFFFFFF);
	
	const Vec128 Sign_PNPN				= VecSet(1.f, -1.f, 1.f, -1.f);
	const Vec128 Sign_PPNN				= VecSet(1.f, 1.f, -1.f, -1.f);
	const Vec128 Sign_NPPN				= VecSet(-1.f, 1.f, 1.f, -1.f);

	const Vec128 Sign_PNNP				= VecSet(1.f, -1.f, -1.f, 1.f);
	const Vec128 Sign_NPNP				= VecSet(-1.f, 1.f, -1.f, 1.f);
	const Vec128 Sign_NNPP				= VecSet(-1.f, -1.f, 1.f, 1.f);

	const Vec128 Vec_One				= VecSet1(1.f);
	const Vec128 Vec_Half				= VecSet1(0.5f);
	const Vec128 Vec_Neg_Half			= VecSet1(-0.5f);
	const Vec128 Vec_1p5				= VecSet1(1.5f);

	const Vec128 Vec_PI					= VecSet1(PI);
	const Vec128 Vec_2_PI				= VecSet1(PI*2.f);
	const Vec128 Vec_Half_PI			= VecSet1(PI*0.5f);
	const Vec128 Vec_1_Over_2_PI		= VecSet1(0.5f / PI);

	const Vec128 Vec_Small_Num			= VecSet1(SMALL_NUMBER);
	
	const Vec128 DegToRad				= VecSet1(PI / 180.f);
	const Vec128 RadToDeg				= VecSet1(180.f / PI);
	
	const Vec128 Half_DegToRad			= VecSet1(0.5f * PI / 180.f);

	namespace _internal_sin
	{
		static const float p = 0.225f;
		static const float a = 16 * sqrt(p);
		static const float b = ((1 - p) / sqrt(p));
	}
	const Vec128 SinParamA				= VecSet1(_internal_sin::a);
	const Vec128 SinParamB				= VecSet1(_internal_sin::b);

	const Vec128 QuatInverseSignMask	= VecSet_i(0x80000000, 0x80000000, 0x80000000, 0x00000000);
};

#define VecLoad1(f_ptr)			_mm_load1_ps(f_ptr)

#define VecAnd(vec1, vec2)		_mm_and_ps(vec1, vec2)
#define VecOr(vec1, vec2)		_mm_or_ps(vec1, vec2)
#define VecXor(vec1, vec2)		_mm_xor_ps(vec1, vec2)

#define VecAdd(vec1, vec2)		_mm_add_ps(vec1, vec2)
#define VecSub(vec1, vec2)		_mm_sub_ps(vec1, vec2)
#define VecMul(vec1, vec2)		_mm_mul_ps(vec1, vec2)
#define VecDiv(vec1, vec2)		_mm_div_ps(vec1, vec2)

//#define VecNegate(vec)			_mm_sub_ps(_mm_setzero_ps(), vec)
#define VecNegate(vec)			_mm_xor_ps(vec, VecConst::SignMask)
#define VecAbs(vec)				_mm_and_ps(vec, VecConst::InvSignMask)

#define VecSqrt(vec)			_mm_sqrt_ps(vec)
#define VecRcp(vec)				_mm_rcp_ps(vec)

#define VecMoveMask(vec)		_mm_movemask_ps(vec)
#define VecCmpLT(vec1, vec2)	_mm_cmplt_ps(vec1, vec2)
#define VecCmpLE(vec1, vec2)	_mm_cmple_ps(vec1, vec2)
#define VecCmpGT(vec1, vec2)	_mm_cmpgt_ps(vec1, vec2)
#define VecCmpGE(vec1, vec2)	_mm_cmpge_ps(vec1, vec2)

#define VecToFloat(vec)			_mm_cvtss_f32(vec)

// mask is Vec128, only highest bit of each f32 component is used
#define VecBlendVar(vec1, vec2, mask)			_mm_blendv_ps(vec1, vec2, mask);
// mask is const int, only lowest 4 bits are used
#define MakeBlendMask(x,y,z,w)				(x | (y<<1) | (z<<2) | (w<<3))
#define VecBlend(vec1, vec2, x,y,z,w)		_mm_blend_ps(vec1, vec2, MakeBlendMask(x,y,z,w));

#define MakeShuffleMask(x,y,z,w)			(x | (y<<2) | (z<<4) | (w<<6))

// vec(0, 1, 2, 3) -> (vec[x], vec[y], vec[z], vec[w])
#define VecSwizzle(vec, x, y, z, w)			_mm_shuffle_ps(vec, vec, MakeShuffleMask(x,y,z,w))
#define VecSwizzle1(vec, x)					_mm_shuffle_ps(vec, vec, MakeShuffleMask(x,x,x,x))
#define VecSwizzleMask(vec, mask)			_mm_shuffle_ps(vec, vec, mask)
// special swizzle
#define VecSwizzle_0101(vec)				_mm_movelh_ps(vec, vec)
#define VecSwizzle_2323(vec)				_mm_movehl_ps(vec, vec)
#define VecSwizzle_0022(vec)				_mm_moveldup_ps(vec)
#define VecSwizzle_1133(vec)				_mm_movehdup_ps(vec)

//			0		1		 2		  3
// return (vec1[x], vec1[y], vec2[z], vec2[w])
#define VecShuffle(vec1, vec2, x, y, z, w)	_mm_shuffle_ps(vec1, vec2, MakeShuffleMask(x,y,z,w))
#define VecShuffleMask(vec1, vec2, mask)	_mm_shuffle_ps(vec1, vec2, mask)
// special shuffle
#define VecShuffle_0101(vec1, vec2)			_mm_movelh_ps(vec1, vec2)
#define VecShuffle_2323(vec1, vec2)			_mm_movehl_ps(vec2, vec1)

// return (vec1[0], vec2[0], vec1[1], vec2[1])
#define VecInterleave_0011(vec1, vec2)			_mm_unpacklo_ps(vec1, vec2)
// return (vec1[2], vec2[2], vec1[3], vec2[3])
#define VecInterleave_2233(vec1, vec2)			_mm_unpackhi_ps(vec1, vec2)

// this is slightly faster at the cost of precision around 2-3 bits of fraction
__forceinline Vec128 VecInvSqrtFast(Vec128 v)
{
	Vec128 x = _mm_rsqrt_ps(v);
	v = _mm_mul_ps(v, VecConst::Vec_Neg_Half);

	// one iteration
	// x1 = x0 * (1.5 - 0.5 * v * x0 * x0)
	Vec128 x2 = _mm_mul_ps(x, x);
	Vec128 t = _mm_add_ps(VecConst::Vec_1p5, _mm_mul_ps(v, x2));
	return _mm_mul_ps(x, t);
}

#if 0
// this is slower than sqrt and div, just for reference
__forceinline Vec128 VecInvSqrtEpic(Vec128 v)
{
	const static Vec128 a = VecSet1(0.5f);
	Vec128 x = _mm_rsqrt_ps(v);
	v = _mm_mul_ps(v, a);

	// Based on unreal implementation, it would make sure rsqrt(1) = 1
	// x1 = x0 + x0 * (0.5 - 0.5 * v * x0 * x0)
	Vec128 x2 = _mm_mul_ps(x, x);
	Vec128 t = _mm_sub_ps(a, _mm_mul_ps(v, x2));
	x = _mm_add_ps(x, _mm_mul_ps(x, t));

	x2 = _mm_mul_ps(x, x);
	t = _mm_sub_ps(a, _mm_mul_ps(v, x2));
	return _mm_add_ps(x, _mm_mul_ps(x, t));
}
#endif

// return float value x1*x2 + y1*y2 + z1*z2 + w1*w2
//#define VecDot(vec1, vec2)		_mm_cvtss_f32(_mm_dp_ps(vec1, vec2, 0xF1))
// This is faster than _mm_dp_ps ?!!!
__forceinline float VecDot(Vec128 vec1, Vec128 vec2)
{
	Vec128 v = _mm_mul_ps(vec1, vec2); // 0, 1, 2, 3
	Vec128 shuf = VecSwizzle_1133(v); // 1, 1, 3, 3
	Vec128 sums = _mm_add_ps(v, shuf); // 0+1, 1+1, 2+3, 3+3
	shuf = VecShuffle_2323(sums, shuf); // 2+3, 3+3, 3, 3
	sums = _mm_add_ps(sums, shuf); //0+1+2+3, x, x, x, 
	return _mm_cvtss_f32(sums);
#if 0
	Vec128 t0 = VecMul(vec1, vec2);
	Vec128 t1 = _mm_hadd_ps(t0, t0);
	Vec128 t2 = _mm_hadd_ps(t1, t1);
	return _mm_cvtss_f32(t2);
#endif
}

__forceinline Vec128 VecDotV(Vec128 vec1, Vec128 vec2)
{
	Vec128 t0 = _mm_mul_ps(vec1, vec2);
	Vec128 t1 = _mm_hadd_ps(t0, t0);
	return _mm_hadd_ps(t1, t1);
}

// for doing vector 3 by itself, we would better do float version
// return float value x1*x2 + y1*y2 + z1*z2
//#define VecDot3(vec1, vec2)		_mm_cvtss_f32(_mm_dp_ps(vec1, vec2, 0x71))
__forceinline float VecDot3(Vec128 vec1, Vec128 vec2)
{
	Vec128 v = _mm_mul_ps(vec1, vec2);// 0, 1, 2, 3
	Vec128 t0 = VecSwizzle_1133(v); // 1, 1, 3, 3
	Vec128 t1 = _mm_add_ps(v, t0); // 0+1, 1+1, 2+3, 3+3
	Vec128 t2 = VecSwizzle_2323(v); // 2, 3, 2, 3
	Vec128 t3 = _mm_add_ps(t1, t2); // 0+1+2, x, x, x
#if 0
	Vec128 t0 = _mm_mul_ps(vec1, vec2); // 0, 1, 2, 3
	Vec128 t1 = _mm_hadd_ps(t0, t0); // 0+1, 2+3, 0+1, 2+3
	Vec128 t2 = VecSwizzle_2323(t0, t0); // 2, 3, 2, 3
	Vec128 t3 = _mm_add_ps(t1, t2); // 0+1+2, x, x, x
#endif
	return _mm_cvtss_f32(t3);
}
__forceinline Vec128 VecDot3V(Vec128 vec1, Vec128 vec2)
{
	Vec128 t0 = _mm_mul_ps(vec1, vec2); // 0, 1, 2, 3
	Vec128 t1 = _mm_hadd_ps(t0, t0); // 0+1, 2+3, 0+1, 2+3
	Vec128 t2 = _mm_add_ps(t0, t1); // x, x, 0+1+2, x
	return VecSwizzle1(t2, 2);
}

// for doing vector 2 by itself, we would better do float version
// return float value x1*x2 + y1*y2
//#define VecDot2(vec1, vec2)		_mm_cvtss_f32(_mm_dp_ps(vec1, vec2, 0x31))
__forceinline float VecDot2(Vec128 vec1, Vec128 vec2)
{
	Vec128 t0 = _mm_mul_ps(vec1, vec2);
	Vec128 t1 = _mm_hadd_ps(t0, t0);
	return _mm_cvtss_f32(t1);
}
__forceinline Vec128 VecDot2V(Vec128 vec1, Vec128 vec2)
{
	Vec128 t0 = _mm_mul_ps(vec1, vec2);
	Vec128 t1 = _mm_hadd_ps(t0, t0); // 0+1, 2+3, 0+1, 2+3
	return VecSwizzle_0022(t1); // duplicate 0, 2 to 1, 3
}

// return vector value (x1*x2 + y1*y2, z1*z2 + w1*w2, ?, ?)
__forceinline Vec128 VecDot2P(Vec128 vec1, Vec128 vec2)
{
	Vec128 v = _mm_mul_ps(vec1, vec2);
	return _mm_hadd_ps(v, v);
}

__forceinline Vec128 VecCross(Vec128 vec1, Vec128 vec2)
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

// won't be correct if input gets too big (over 1.0e+6)
__forceinline Vec128 VecSin(Vec128 vec)
{
	// based on http://forum.devmaster.net/t/fast-and-accurate-sine-cosine/9648
	// based on unreal implementation of constants
	// y = 8x - 16x|x| = 16 * x * (0.5 - |x|)
	// sin = p*y|y| + (1-p)*y = p*y*(|y| - (1-p)/p) = sqrt(p)*y*(|sqrt(p)*y| + (1-p)/sqrt(p))
	
	Vec128 x = _mm_mul_ps(vec, VecConst::Vec_1_Over_2_PI); // map input period to [0, 1]
	x = _mm_sub_ps(x, _mm_round_ps(_mm_add_ps(x, VecConst::Vec_Half), _MM_FROUND_FLOOR)); // fix range to [-0.5, 0.5]
	Vec128 y = _mm_mul_ps(VecConst::SinParamA, _mm_mul_ps(x, _mm_sub_ps(VecConst::Vec_Half, VecAbs(x))));
	return _mm_mul_ps(y, _mm_add_ps(VecAbs(y), VecConst::SinParamB));
}

__forceinline Vec128 VecCos(Vec128 vec)
{
	return VecSin(_mm_add_ps(vec, VecConst::Vec_Half_PI));
}

// Matrix 2x2 operations
//
// for column major matrix ( specific functions surfix by _CM )
// we use Vec128 to represent 2x2 matrix as A = | A0  A2 |
//                                              | A1  A3 |
//
// for row major matrix ( specific functions surfix by _RM )
// we use Vec128 to represent 2x2 matrix as A = | A0  A1 |
//                                              | A2  A3 |

// 2x2 column major Matrix multiply A*B
__forceinline Vec128 Mat2Mul_CM(Vec128 vec1, Vec128 vec2)
{
	return 
		_mm_add_ps(	_mm_mul_ps(						vec1, VecSwizzle(vec2, 0,0,3,3)),
					_mm_mul_ps(VecSwizzle(vec1, 2,3,0,1), VecSwizzle(vec2, 1,1,2,2)));
}
// 2x2 row major Matrix multiply A*B
__forceinline Vec128 Mat2Mul_RM(Vec128 vec1, Vec128 vec2)
{
	return 
		_mm_add_ps(	_mm_mul_ps(						vec1, VecSwizzle(vec2, 0,3,0,3)),
					_mm_mul_ps(VecSwizzle(vec1, 1,0,3,2), VecSwizzle(vec2, 2,1,2,1)));
}

// 2x2 column major Matrix adjugate multiply adj(A)*B
__forceinline Vec128 Mat2AdjMul_CM(Vec128 vec1, Vec128 vec2)
{
	return
		_mm_sub_ps(	_mm_mul_ps(VecSwizzle(vec1, 3,0,3,0), vec2),
					_mm_mul_ps(VecSwizzle(vec1, 2,1,2,1), VecSwizzle(vec2, 1,0,3,2)));

}
// 2x2 row major Matrix adjugate multiply adj(A)*B
__forceinline Vec128 Mat2AdjMul_RM(Vec128 vec1, Vec128 vec2)
{
	return
		_mm_sub_ps(	_mm_mul_ps(VecSwizzle(vec1, 3,3,0,0), vec2),
					_mm_mul_ps(VecSwizzle(vec1, 1,1,2,2), VecSwizzle(vec2, 2,3,0,1)));

}
// 2x2 column major Matrix multiply adjugate A*adj(B)
__forceinline Vec128 Mat2MulAdj_CM(Vec128 vec1, Vec128 vec2)
{
	return
		_mm_sub_ps(	_mm_mul_ps(						vec1, VecSwizzle(vec2, 3,3,0,0)),
					_mm_mul_ps(VecSwizzle(vec1, 2,3,0,1), VecSwizzle(vec2, 1,1,2,2)));
}
// 2x2 row major Matrix multiply adjugate A*adj(B)
__forceinline Vec128 Mat2MulAdj_RM(Vec128 vec1, Vec128 vec2)
{
	return
		_mm_sub_ps(	_mm_mul_ps(						vec1, VecSwizzle(vec2, 3,0,3,0)),
					_mm_mul_ps(VecSwizzle(vec1, 1,0,3,2), VecSwizzle(vec2, 2,1,2,1)));
}