#pragma once

#include "MathUtil.h"

struct Vector4;
typedef Vector4 Vector4_3;
typedef Vector4 Vector4_2;

__declspec(align(16)) struct Vector4
{
public:
	union
	{
		float m[4];
		Vec128 m128;

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

	Vector4(float inX) :
		x(inX), y(inX), z(inX), w(inX)
	{}

	Vector4(Vector4_3 v, float inW) :
		x(v.x), y(v.y), z(v.z), w(inW)
	{}

	Vector4(Vec128 vec)
	{
		m128 = vec;
	}

	// const getter
	static __forceinline Vector4 Zero() { return VecZero(); }


	// add
	inline Vector4 operator+(const Vector4& v) const
	{
		return VecAdd(m128, v.m128);
	}
	inline Vector4 operator+(float f) const
	{
		return VecAdd(m128, VecSet1(f));
	}
	inline Vector4& operator+=(const Vector4& v)
	{
		m128 = VecAdd(m128, v.m128);
		return *this;
	}
	inline Vector4& operator+=(float f)
	{
		m128 = VecAdd(m128, VecSet1(f));
		return *this;
	}

	// sub
	inline Vector4 operator-(const Vector4& v) const
	{
		return VecSub(m128, v.m128);
	}
	inline Vector4 operator-(float f) const
	{
		return VecSub(m128, VecSet1(f));
	}
	inline Vector4& operator-=(const Vector4& v)
	{
		m128 = VecSub(m128, v.m128);
		return *this;
	}
	inline Vector4& operator-=(float f)
	{
		m128 = VecSub(m128, VecSet1(f));
		return *this;
	}

	// mul
	inline Vector4 operator*(const Vector4& v) const
	{
		return VecMul(m128, v.m128);
	}
	inline Vector4 operator*(float f) const
	{
		return VecMul(m128, VecSet1(f));
	}
	inline Vector4& operator*=(const Vector4& v)
	{
		m128 = VecMul(m128, v.m128);
		return *this;
	}
	inline Vector4& operator*=(float f)
	{
		m128 = VecMul(m128, VecSet1(f));
		return *this;
	}

	// div
	inline Vector4 operator/(const Vector4& v) const
	{
		return VecDiv(m128, v.m128);
	}
	inline Vector4 operator/(float f) const
	{
		return VecDiv(m128, VecSet1(f));
	}
	inline Vector4& operator/=(const Vector4& v)
	{
		m128 = VecDiv(m128, v.m128);
		return *this;
	}
	inline Vector4& operator/=(float f)
	{
		m128 = VecDiv(m128, VecSet1(f));
		return *this;
	}

	// negate
	inline Vector4 operator-() const
	{
		return VecNegate(m128);
	}

	// vector4 dot: return float value x1*x2 + y1*y2 + z1*z2 + w1*w2
	inline float Dot4(const Vector4& v) const
	{
		return VecDot(m128, v.m128);
	}

	// vector4_3 dot: return float value x1*x2 + y1*y2 + z1*z2
	inline float Dot3(const Vector4_3& v) const
	{
		// float version is faster
		return x*v.x + y*v.y + z*v.z;
		//return VecDot3(m128, v.m128);
	}

	// vector4_2 dot: return float value x1*x2 + y1*y2
	inline float Dot2(const Vector4_2& v) const
	{
		// float version is faster
		return x*v.x + y*v.y;
		//return VecDot2(m128, v.m128);
	}

	// vector4 dot: return float value x1*x2 + y1*y2 + z1*z2 + w1*w2
	static inline float Dot4(const Vector4& v1, const Vector4& v2) { return v1.Dot4(v2); }
	// vector4_3 dot: return float value x1*x2 + y1*y2 + z1*z2
	static inline float Dot3(const Vector4_3& v1, const Vector4_3& v2) { return v1.Dot3(v2); }
	// vector4_2 dot: return float value x1*x2 + y1*y2
	static inline float Dot2(const Vector4_2& v1, const Vector4_2& v2) { return v1.Dot2(v2); }

	// vector4_3 cross: w can be anyvalue based on implementation
	inline Vector4_3 Cross3(const Vector4_3& v) const
	{
		//return Vector4(y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x);
		return VecCross(m128, v.m128);
	}

	// vector4_2 cross: return z value as a result of Cross3
	inline float Cross2(const Vector4_2& v) const
	{
		return x*v.y - y*v.x;
	}

	// vector4_3 cross: w can be anyvalue based on implementation
	static inline Vector4_3 Cross3(const Vector4_3& v1, const Vector4_3& v2) { return v1.Cross3(v2); }
	// vector4_2 cross: return z value as a result of Cross3
	static inline float Cross2(const Vector4_2& v1, const Vector4_2& v2) { return v1.Cross2(v2); }

	// size
	inline float SizeSqr4() const
	{
		return Dot4(*this);
	}
	inline float SizeSqr3() const
	{
		return Dot3(*this);
	}
	inline float SizeSqr2() const
	{
		return Dot2(*this);
	}
	inline float Size4() const
	{
		return sqrtf(Dot4(*this));
	}
	inline float Size3() const
	{
		return sqrtf(Dot3(*this));
	}
	inline float Size2() const
	{
		return sqrtf(Dot2(*this));
	}

	// normalize
	// Vector4 version time: SSE < normal
	inline Vector4 GetNormalized4() const
	{
		// sse method without branch
		Vec128 sizeSqr = VecDotV(m128, m128);
		// if sizeSqr < SMALL_NUMBER return 0, else normalize
		return VecBlendVar(
			VecDiv(m128, VecSqrt(sizeSqr)),
			VecZero(),
			VecCmpLT(sizeSqr, VecConst::Vec_Small_Num));

#if 0 // normal method, sizeSqr is a float
		float sizeSqr = Dot4(*this);
		return (sizeSqr < SMALL_NUMBER) ?
			VecZero() :
			VecDiv(m128, VecSet1(sqrt(sizeSqr)));
#endif
	}
	// precision around 0.0003
	inline Vector4 GetNormalized4Fast() const
	{
		// sse method without branch
		Vec128 sizeSqr = VecDotV(m128, m128);
		// if sizeSqr < SMALL_NUMBER return 0, else normalize
		return VecBlendVar(
			VecMul(m128, VecInvSqrtFast(sizeSqr)),
			VecZero(),
			VecCmpLT(sizeSqr, VecConst::Vec_Small_Num));
	}

	// Vector4_3 version time: SSE(set1(Dot3)) < SSE(VecDot3V) < normal
	// only take x,y,z; w will set to 0
	inline Vector4 GetNormalized3() const
	{
		// set1 is faster than caclculate dot3 as vec128
		Vec128 sizeSqr = VecSet1(Dot3(*this));
		//Vec128 sizeSqr = VecDot3V(m128, m128);

		// if sizeSqr < SMALL_NUMBER return 0, else normalize
		// also mask w value so it's always 0 (w mask value all 1)
		return VecBlendVar(
			VecDiv(m128, VecSqrt(sizeSqr)),
			VecZero(),
			VecZeroW(VecCmpLT(sizeSqr, VecConst::Vec_Small_Num)));
	}
	
	// Vector4_2 version time: SSE(set1(Dot2)) < normal < SSE(VecDot2V)
	// only take x,y; z,w will set to 0
	inline Vector4 GetNormalized2() const
	{
		// set1 is faster than caclculate dot3 as vec128
		Vec128 sizeSqr = VecSet1(Dot2(*this));
		//Vec128 sizeSqr = VecDot2V(m128, m128);

		// if sizeSqr < SMALL_NUMBER return 0, else normalize
		// also mask z,w value so it's always 0 (zw mask value all 1)
		return VecBlendVar(
			VecDiv(m128, VecSqrt(sizeSqr)),
			VecZero(),
			VecZeroZW(VecCmpLT(sizeSqr, VecConst::Vec_Small_Num)));
	}
};