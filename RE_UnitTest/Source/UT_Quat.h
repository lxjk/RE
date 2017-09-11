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