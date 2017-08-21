#pragma once

#include "MathUtil.h"

#include "Vector4.h"

struct Matrix4;

// Column major matrix 4x4
__declspec(align(16))
struct Matrix4
{
public:
	union
	{
		float m[4][4];
		Vec128 mVec[4];
		Vector4 mColumns[4];
	};
};