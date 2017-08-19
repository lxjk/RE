#pragma once

#include "Math/Vector4.h"

inline Vector4 UT_Vector4_Add(const Vector4& v1, const Vector4& v2)
{
	return Vector4(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w);
}
inline Vector4 UT_Vector4_Add(const Vector4& v1, float f)
{
	return Vector4(v1.x + f, v1.y + f, v1.z + f, v1.w + f);
}