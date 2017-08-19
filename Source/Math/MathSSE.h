#pragma once

#include <emmintrin.h>

typedef __m128	Vec128;

#define VecLoad1(f_ptr)			_mm_load1_ps(f_ptr)

#define VecSet1(f)				_mm_set1_ps(f)

#define VecAdd(vec1, vec2)		_mm_add_ps(vec1, vec2)

#define VecSub(vec1, vec2)		_mm_sub_ps(vec1, vec2)

#define VecMul(vec1, vec2)		_mm_mul_ps(vec1, vec2)

#define VecDiv(vec1, vec2)		_mm_div_ps(vec1, vec2)

#define VecNegate(vec)			_mm_sub_ps(_mm_setzero_ps(), vec)