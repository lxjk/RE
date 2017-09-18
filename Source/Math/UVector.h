#pragma once

#include "Vector4.h"

// unaligned vector class, should only used for packed storage
struct UVector4
{
public:
	union
	{
		float m[4];
		struct
		{
			float x, y, z, w;
		};
	};

	UVector4() {};
	UVector4(float inX, float inY, float inZ, float inW) :
		x(inX), y(inY), z(inZ), w(inW)
	{}

	UVector4(float inX) :
		x(inX), y(inX), z(inX), w(inX)
	{}

	UVector4(const Vector4& v) :
		x(v.x), y(v.y), z(v.z), w(v.w)
	{}

	Vector4 ToVector4()
	{
		return Vector4(x, y, z, w);
	}
};

// unaligned vector class, should only used for packed storage
struct UVector3
{
public:
	union
	{
		float m[3];
		struct
		{
			float x, y, z;
		};
	};

	UVector3() {};
	UVector3(float inX, float inY, float inZ) :
		x(inX), y(inY), z(inZ)
	{}

	UVector3(float inX) :
		x(inX), y(inX), z(inX)
	{}

	UVector3(const Vector4& v) :
		x(v.x), y(v.y), z(v.z)
	{}

	Vector4 ToVector4()
	{
		return Vector4(x, y, z);
	}
};

// unaligned vector class, should only used for packed storage
struct UVector2
{
public:
	union
	{
		float m[2];
		struct
		{
			float x, y;
		};
	};

	UVector2() {};
	UVector2(float inX, float inY) :
		x(inX), y(inY)
	{}

	UVector2(float inX) :
		x(inX), y(inX)
	{}

	UVector2(const Vector4& v) :
		x(v.x), y(v.y)
	{}

	Vector4 ToVector4()
	{
		return Vector4(x, y);
	}
};