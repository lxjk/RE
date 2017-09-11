#pragma once

#undef  PI
#define PI 					(3.1415926535897932f)
#define SMALL_NUMBER		(1.e-8f)
#define KINDA_SMALL_NUMBER	(1.e-4f)

#include <Math.h>

#include "MathSSE.h"

union IntFloatUnion
{
	unsigned __int32 u;
	__int32 i;
	float f;
};