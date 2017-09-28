#pragma once

#include <immintrin.h>

#include "MathSSE.h"

typedef __m256	Vec256;


#define Vec256Set1(f)					_mm256_set1_ps(f)
#define Vec256Set1_i(i)					_mm256_castsi256_ps(_mm256_set1_epi32(i))
#define Vec256Set1_Vec128(v)			_mm256_set_m128(v, v)

#define Vec256Set_Vec128(v1, v2)		_mm256_setr_m128(v1, v2)

namespace Vec256Const {

	const Vec256 SignMask				= Vec256Set1_i(0x80000000);
	const Vec256 InvSignMask			= Vec256Set1_i(0x7FFFFFFF);

	const Vec256 Vec_One				= Vec256Set1(1.f);
	const Vec256 Vec_Half				= Vec256Set1(0.5f);

	const Vec256 Vec_PI					= Vec256Set1(PI);
	const Vec256 Vec_2_PI				= Vec256Set1(PI*2.f);
	const Vec256 Vec_Half_PI			= Vec256Set1(PI*0.5f);
	const Vec256 Vec_1_Over_2_PI		= Vec256Set1(0.5f / PI);
};

// zero upper for ALL ymm registers
// DO THIS before going back to SSE
#define Vec256ZeroUpper()			_mm256_zeroupper()

#define Vec256Abs(vec)				_mm256_and_ps(vec, Vec256Const::InvSignMask)

// re-define VecSwizzle using new instruction
#undef VecSwizzle
// vec(0, 1, 2, 3) -> (vec[x], vec[y], vec[z], vec[w])
#define VecSwizzle(vec, x, y, z, w)					_mm_permute_ps(vec, MakeShuffleMask(x,y,z,w))
// swizzle two 128 lanes independently
#define Vec256Swizzle(vec, x, y, z, w)				_mm256_permute_ps(vec, MakeShuffleMask(x,y,z,w))

// shuffle two 128 lanes independently
// return (v1(0)[x], v1(0)[y], v2(0)[z], v2(0)[w]; v1(1)[x], v1(1)[y], v2(1)[z], v2(1)[w])
#define Vec256Shuffle(vec1, vec2, x, y, z, w)		_mm256_shuffle_ps(vec1, vec2, MakeShuffleMask(x,y,z,w))

// select returns 128f: 0 -> vec1(0), 1 -> vec1(1), 2 -> vec2(0), 3 -> vec2(1), 8 -> 0
// return ( select(x), select(y) )
#define Vec256SelectOrZero(vec1, vec2, x, y)		_mm256_permute2f128_ps(vec1, vec2, (x) | (y<<4))


// same method as VecSin in MathSSE.h
__forceinline void VecSinCos(Vec128 vec, Vec128& outSin, Vec128& outCos)
{
	const Vec256 SinParamA = Vec256Set1(7.58946638440411f);
	const Vec256 SinParamB = Vec256Set1(1.63384345775366f);

	Vec256 x = Vec256Set_Vec128(vec, _mm_add_ps(vec, VecConst::Vec_Half_PI));

	x = _mm256_mul_ps(x, Vec256Const::Vec_1_Over_2_PI); // map input period to [0, 1]
	x = _mm256_sub_ps(x, _mm256_round_ps(_mm256_add_ps(x, Vec256Const::Vec_Half), _MM_FROUND_FLOOR)); // fix range to [-0.5, 0.5]
	Vec256 y = _mm256_mul_ps(SinParamA, _mm256_mul_ps(x, _mm256_sub_ps(Vec256Const::Vec_Half, Vec256Abs(x))));
	y = _mm256_mul_ps(y, _mm256_add_ps(Vec256Abs(y), SinParamB));
	outCos = _mm256_extractf128_ps(y, 1);
	outSin = _mm256_castps256_ps128(y);
}