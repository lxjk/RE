#pragma once

#include "MathUtil.h"

#include "Vector4.h"

struct Quat;

// Quaternion
// (v.x*sin(theta/2), v.y*sin(theta/2), v.z*sin(theta/2), cos(theta/2))
__declspec(align(16)) struct Quat
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
	};

	Quat() {};

	Quat(float inX, float inY, float inZ, float inW) :
		x(inX), y(inY), z(inZ), w(inW)
	{}

	Quat(Vec128 vec128)
	{
		m128 = vec128;
	}

	static __forceinline Quat Identity()
	{
		const Quat identity(VecSet(0.f, 0.f, 0.f, 1.f));
		return identity;
	}

	// add
	inline Quat operator+(const Quat& q) const
	{
		return VecAdd(m128, q.m128);
	}
	inline Quat& operator+=(const Quat& q)
	{
		m128 = VecAdd(m128, q.m128);
		return *this;
	}
	
	// sub
	inline Quat operator-(const Quat& q) const
	{
		return VecSub(m128, q.m128);
	}
	inline Quat& operator-=(const Quat& q)
	{
		m128 = VecSub(m128, q.m128);
		return *this;
	}

	// mul
	inline Quat operator*(float f) const
	{
		return VecMul(m128, VecSet1(f));
	}
	inline Quat& operator*=(float f)
	{
		m128 = VecMul(m128, VecSet1(f));
		return *this;
	}

	// div
	inline Quat operator/(float f) const
	{
		return VecDiv(m128, VecSet1(f));
	}
	inline Quat& operator/=(float f)
	{
		m128 = VecDiv(m128, VecSet1(f));
		return *this;
	}

	// negate
	inline Quat operator-() const
	{
		return VecNegate(m128);
	}

	// size
	inline float SizeSqr() const
	{
		return VecDot(m128, m128);
	}
	inline float Size() const
	{
		return sqrtf(SizeSqr());
	}

	// normalize
	inline Quat GetNormalized() const
	{
		// sse method without branch
		Vec128 sizeSqr = VecDotV(m128, m128);
		// if sizeSqr < SMALL_NUMBER return 0, else normalize
		return VecBlendVar(
			VecDiv(m128, VecSqrt(sizeSqr)),
			VecZero(),
			VecCmpLT(sizeSqr, VecConst::Vec_Small_Num));
	}

	// mul quat
	inline Quat operator*(const Quat& q) const
	{
		Vec128 r =				VecMul(VecSwizzle1(m128, 3), q.m128);
		r = VecAdd(r, VecMul(	VecMul(VecSwizzle1(m128, 0), VecSwizzle(q.m128, 3,2,1,0)), VecConst::Sign_PNPN));
		r = VecAdd(r, VecMul(	VecMul(VecSwizzle1(m128, 1), VecSwizzle(q.m128, 2,3,0,1)), VecConst::Sign_PPNN));
		r = VecAdd(r, VecMul(	VecMul(VecSwizzle1(m128, 2), VecSwizzle(q.m128, 1,0,3,2)), VecConst::Sign_NPPN));

		return r;

#if 0 // the version above is faster
		const Vec128 quatSignMask = VecSet(1.f, 1.f, 1.f, -1.f);

		Vec128 r;
		r = VecMul(VecSwizzle_0101(m128), VecSwizzle(q.m128, 3, 3, 1, 1));
		r = VecAdd(r, VecMul(VecSwizzle(m128, 3, 2, 3, 2), VecSwizzle_0022(q.m128)));
		r = VecAdd(r, VecMul(VecSwizzle(m128, 1, 3, 2, 0), VecSwizzle(q.m128, 2, 1, 3, 0)));
		r = VecSub(r, VecMul(VecSwizzle(m128, 2, 0, 1, 3), VecSwizzle(q.m128, 1, 2, 0, 3)));

		return VecMul(r, quatSignMask);
#endif
	}
	inline Quat& operator*=(const Quat& q)
	{
		m128 =						VecMul(VecSwizzle1(m128, 3), q.m128);
		m128 = VecAdd(m128, VecMul(	VecMul(VecSwizzle1(m128, 0), VecSwizzle(q.m128, 3,2,1,0)), VecConst::Sign_PNPN));
		m128 = VecAdd(m128, VecMul(	VecMul(VecSwizzle1(m128, 1), VecSwizzle(q.m128, 2,3,0,1)), VecConst::Sign_PPNN));
		m128 = VecAdd(m128, VecMul(	VecMul(VecSwizzle1(m128, 2), VecSwizzle(q.m128, 1,0,3,2)), VecConst::Sign_NPPN));

		return *this;
	}

	// inverse assume this quaterion is normalized
	inline Quat GetInverse() const
	{
		const Vec128 QuatInverseSignMask = CastVeciToVec(VeciSet(0x80000000, 0x80000000, 0x80000000, 0x00000000));
		return VecXor(m128, QuatInverseSignMask);
	}

	//   v + 2(q x (q x v + w*p))
	// = v + q x (2(q x v)) + w*(2(q x v))
	inline Vector4_3 Rotate(const Vector4_3& v) const
	{
		Vec128 t0 = VecCross(m128, v.m128);		// q x v
		Vec128 t1 = VecAdd(t0, t0);				// 2(q x v)
		Vec128 t2 = VecCross(m128, t1);			// q x (2(q x v))
		Vec128 t3 = VecAdd(v.m128, VecMul(VecSwizzle1(m128, 3), t1)); // v + w*(2(q x v))
		return VecAdd(t2, t3);
	}
	// = v + q x (2(q x v)) - w*(2(q x v))
	inline Vector4_3 InverseRotate(const Vector4_3& v) const
	{
		Vec128 t0 = VecCross(m128, v.m128);		// q x v
		Vec128 t1 = VecAdd(t0, t0);				// 2(q x v)
		Vec128 t2 = VecCross(m128, t1);			// q x (2(q x v))
		Vec128 t3 = VecSub(v.m128, VecMul(VecSwizzle1(m128, 3), t1)); // v - w*(2(q x v))
		return VecAdd(t2, t3);
	}

};