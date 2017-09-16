#pragma once

#undef  PI
#define PI 					(3.1415926535897932f)
#define SMALL_NUMBER		(1.e-8f)
#define KINDA_SMALL_NUMBER	(1.e-4f)

#define ENABLE_VEC256 0

#include <Math.h>

#include "MathSSE.h"
#if ENABLE_VEC256
#include "MathAVX.h"
#endif

union IntFloatUnion
{
	unsigned __int32 u;
	__int32 i;
	float f;
};