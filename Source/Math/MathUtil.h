#pragma once

#include "MathSSE.h"

#undef  PI
#define PI 					(3.1415926535897932f)
#define SMALL_NUMBER		(1.e-8f)
#define KINDA_SMALL_NUMBER	(1.e-4f)

union IntFloatUnion
{
	__int32 i;
	unsigned __int32 u;
	float f;
};