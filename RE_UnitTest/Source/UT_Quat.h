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