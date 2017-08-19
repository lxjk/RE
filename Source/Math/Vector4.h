#pragma once

#include "MathUtil.h"

__declspec(align(16))
struct Vector4
{
public:
	union
	{
		float m[4];
		Vec128 mVec;

		struct
		{
			float x, y, z, w;
		};
		struct
		{
			float r, g, b, a;
		};
	};

	Vector4() {};

	Vector4(float inX, float inY, float inZ = 0, float inW = 0) :
		x(inX), y(inY), z(inZ), w(inW)
	{}

	Vector4(Vec128& vec128)
	{
		mVec = vec128;
	}

	inline Vector4 operator+(const Vector4& v) const
	{
		return VecAdd(mVec, v.mVec);
	}
	inline Vector4 operator+(float f) const
	{
		return VecAdd(mVec, VecSet1(f));
	}

	inline Vector4 operator-(const Vector4& v) const
	{
		return VecSub(mVec, v.mVec);
	}
	inline Vector4 operator-(float f) const
	{
		return VecSub(mVec, VecSet1(f));
	}

	inline Vector4 operator*(const Vector4& v) const
	{
		return VecMul(mVec, v.mVec);
	}
	inline Vector4 operator*(float f) const
	{
		return VecMul(mVec, VecSet1(f));
	}

	inline Vector4 operator/(const Vector4& v) const
	{
		return VecDiv(mVec, v.mVec);
	}
	inline Vector4 operator/(float f) const
	{
		return VecDiv(mVec, VecSet1(f));
	}

	inline Vector4 operator-() const
	{
		return VecNegate(mVec);
	}
};