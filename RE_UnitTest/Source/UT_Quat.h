#pragma once

#include "../../3rdparty/glm/glm/glm.hpp"
#include "../../3rdparty/glm/glm/gtc/quaternion.hpp"
#include "Math/Quat.h"
#include "UT_Vector4.h"


#define QuatToGlmQuat(value) *(glm::quat*)(&value)
#define GlmQuatToQuat(value) *(Quat*)(&value)


inline Quat UT_Quat_Mul(const Quat& q1, const Quat& q2)
{
	return Quat(
		q1.x * q2.w + q1.w * q2.x + q1.y * q2.z - q1.z * q2.y,
		q1.y * q2.w + q1.w * q2.y + q1.z * q2.x - q1.x * q2.z,
		q1.z * q2.w + q1.w * q2.z + q1.x * q2.y - q1.y * q2.x,
		q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z
		);
}

inline Quat UT_Quat_Mul_Glm(const Quat& q1, const Quat& q2)
{
	glm::quat r = QuatToGlmQuat(q1) * QuatToGlmQuat(q2);
	return GlmQuatToQuat(r);
}


inline Vector4 UT_Quat_Rotate_Glm(const Quat& q1, const Vector4& v2)
{
	glm::vec3 r = QuatToGlmQuat(q1) * Vector4ToGlmVec3(v2);
	return GlmVec3ToVector4(r);
}

inline Vector4 UT_Quat_InverseRotate_Glm(const Quat& q1, const Vector4& v2)
{
	glm::vec3 r = Vector4ToGlmVec3(v2) * QuatToGlmQuat(q1);
	return GlmVec3ToVector4(r);
}

inline Quat UT_Matrix4ToQuat(const Matrix4& m)
{
	Quat r;
	float	s;

	// Check diagonal (trace)
	const float tr = m.m[0][0] + m.m[1][1] + m.m[2][2];

	if (tr > 0.0f)
	{
		float InvS = 1.f / sqrt(tr + 1.f);
		r.w = 0.5f * (1.f / InvS);
		s = 0.5f * InvS;

		r.x = (m.m[1][2] - m.m[2][1]) * s;
		r.y = (m.m[2][0] - m.m[0][2]) * s;
		r.z = (m.m[0][1] - m.m[1][0]) * s;
	}
	else
	{
		// diagonal is negative
		int i = 0;

		if (m.m[1][1] > m.m[0][0])
			i = 1;

		if (m.m[2][2] > m.m[i][i])
			i = 2;

		static const int nxt[3] = { 1, 2, 0 };
		const int j = nxt[i];
		const int k = nxt[j];

		s = m.m[i][i] - m.m[j][j] - m.m[k][k] + 1.0f;

		float InvS = 1.f / sqrt(s);

		float qt[4];
		qt[i] = 0.5f * (1.f / InvS);

		s = 0.5f * InvS;

		qt[3] = (m.m[j][k] - m.m[k][j]) * s;
		qt[j] = (m.m[i][j] + m.m[j][i]) * s;
		qt[k] = (m.m[i][k] + m.m[k][i]) * s;

		r.x = qt[0];
		r.y = qt[1];
		r.z = qt[2];
		r.w = qt[3];
	}
	return r;
}


inline Quat UT_EulerToQuat(const Vector4& v)
{
	Vector4 halfAngle = v * (0.5f * PI / 180.f);

	Vector4 s(sinf(halfAngle.x), sinf(halfAngle.y), sinf(halfAngle.z), 0);
	Vector4 c(cosf(halfAngle.x), cosf(halfAngle.y), cosf(halfAngle.z), 0);

	Quat q;
	q.w = c.x * c.y * c.z + s.x * s.y * s.z;
	q.x = s.x * c.y * c.z - c.x * s.y * s.z;
	q.y = c.x * s.y * c.z + s.x * c.y * s.z;
	q.z = c.x * c.y * s.z - s.x * s.y * c.z;

	return q;
}

inline Quat UT_EulerToQuat_Glm(const Vector4& v)
{
	glm::quat t = glm::quat(glm::radians(Vector4ToGlmVec3(v)));
	Quat q(t.x, t.y, t.z, t.w);
	return q;
}

// euler is in degrees
inline Vector4_3 UT_QuatToEuler(const Quat& q)
{
	float x, y, z;

#if EULER_ANGLE==EULER_ZXY
	float t0 = 2.f * (q.y*q.z + q.x*q.w);
	if (abs(t0) > 1.f - SMALL_NUMBER)
	{
		x = PI * (t0 > 0.f ? 0.5f : -0.5f);
		z = 0.f;
		y = 2.f * atan2f(q.y, q.w);
	}
	else
	{
		x = asinf(t0);
		z = atan2f(2.f * (q.z*q.w - q.x*q.y), 1.f - 2.f * (q.x*q.x + q.z*q.z));
		y = atan2f(2.f * (q.y*q.w - q.x*q.z), 1.f - 2.f * (q.x*q.x + q.y*q.y));
	}
#else
	// default to EULER_ZYX
	float t0 = 2.f * (q.y*q.w - q.x*q.z);
	if (abs(t0) > 1.f - SMALL_NUMBER)
	{
		y = PI * (t0 > 0.f ? 0.5f : -0.5f);
		z = 0.f;
		x = 2.f * atan2f(q.x, q.w);
	}
	else
	{
		y = asinf(t0);
		z = atan2f(2.f * (q.x*q.y + q.z*q.w), 1.f - 2.f * (q.y*q.y + q.z*q.z));
		x = atan2f(2.f * (q.y*q.z + q.x*q.w), 1.f - 2.f * (q.x*q.x + q.y*q.y));
	}
#endif

	return VecMul(VecSet(x, y, z, 0), VecConst::RadToDeg);
}