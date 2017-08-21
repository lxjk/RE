#pragma once

#include "../../3rdparty/glm/glm/glm.hpp"
#include "Math/Vector4.h"

#define Vector4ToGlmVec4Named(name, value) glm::vec4 name(value.x, value.y, value.z, value.w)
#define Vector4ToGlmVec4(value) glm::vec4(value.x, value.y, value.z, value.w)
#define Vector4ToGlmVec3(value) glm::vec3(value.x, value.y, value.z)
#define Vector4ToGlmVec2(value) glm::vec2(value.x, value.y)
#define GlmVec4ToVector4Named(name, value) Vector4 name(value.x, value.y, value.z, value.w)
#define GlmVec4ToVector4(value) Vector4(value.x, value.y, value.z, value.w)
#define GlmVec3ToVector4(value) Vector4(value.x, value.y, value.z, 0)
#define GlmVec2ToVector4(value) Vector4(value.x, value.y, 0, 0)

inline Vector4 UT_Vector4_Add(const Vector4& v1, const Vector4& v2)
{
	return Vector4(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w);
}
inline Vector4 UT_Vector4_Add_Glm(const Vector4& v1, const Vector4& v2)
{
	glm::vec4 r = Vector4ToGlmVec4(v1) + Vector4ToGlmVec4(v2);
	return GlmVec4ToVector4(r);
}
inline Vector4 UT_Vector4_Add(const Vector4& v1, float f)
{
	return Vector4(v1.x + f, v1.y + f, v1.z + f, v1.w + f);
}
inline Vector4 UT_Vector4_Add_Glm(const Vector4& v1, float f)
{
	glm::vec4 r = Vector4ToGlmVec4(v1) + f;
	return GlmVec4ToVector4(r);
}

inline Vector4 UT_Vector4_Sub(const Vector4& v1, const Vector4& v2)
{
	return Vector4(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w);
}
inline Vector4 UT_Vector4_Sub_Glm(const Vector4& v1, const Vector4& v2)
{
	glm::vec4 r = Vector4ToGlmVec4(v1) - Vector4ToGlmVec4(v2);
	return GlmVec4ToVector4(r);
}
inline Vector4 UT_Vector4_Sub(const Vector4& v1, float f)
{
	return Vector4(v1.x - f, v1.y - f, v1.z - f, v1.w - f);
}
inline Vector4 UT_Vector4_Sub_Glm(const Vector4& v1, float f)
{
	glm::vec4 r = Vector4ToGlmVec4(v1) - f;
	return GlmVec4ToVector4(r);
}

inline Vector4 UT_Vector4_Mul(const Vector4& v1, const Vector4& v2)
{
	return Vector4(v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.w * v2.w);
}
inline Vector4 UT_Vector4_Mul_Glm(const Vector4& v1, const Vector4& v2)
{
	glm::vec4 r = Vector4ToGlmVec4(v1) * Vector4ToGlmVec4(v2);
	return GlmVec4ToVector4(r);
}
inline Vector4 UT_Vector4_Mul(const Vector4& v1, float f)
{
	return Vector4(v1.x * f, v1.y * f, v1.z * f, v1.w * f);
}
inline Vector4 UT_Vector4_Mul_Glm(const Vector4& v1, float f)
{
	glm::vec4 r = Vector4ToGlmVec4(v1) * f;
	return GlmVec4ToVector4(r);
}

inline Vector4 UT_Vector4_Div(const Vector4& v1, const Vector4& v2)
{
	return Vector4(v1.x / v2.x, v1.y / v2.y, v1.z / v2.z, v1.w / v2.w);
}
inline Vector4 UT_Vector4_Div_Glm(const Vector4& v1, const Vector4& v2)
{
	glm::vec4 r = Vector4ToGlmVec4(v1) / Vector4ToGlmVec4(v2);
	return GlmVec4ToVector4(r);
}
inline Vector4 UT_Vector4_Div(const Vector4& v1, float f)
{
	return Vector4(v1.x / f, v1.y / f, v1.z / f, v1.w / f);
}
inline Vector4 UT_Vector4_Div_Glm(const Vector4& v1, float f)
{
	glm::vec4 r = Vector4ToGlmVec4(v1) / f;
	return GlmVec4ToVector4(r);
}

inline Vector4 UT_Vector4_Negate(const Vector4& v1)
{
	return Vector4(-v1.x, -v1.y, -v1.z, -v1.w);
}
inline Vector4 UT_Vector4_Negate_Glm(const Vector4& v1)
{
	glm::vec4 r = -Vector4ToGlmVec4(v1);
	return GlmVec4ToVector4(r);
}

inline float UT_Vector4_Dot(const Vector4& v1, const Vector4& v2)
{
	// somehow the parenthese are crucial to get the same value as SSE or glm version
	return (v1.x * v2.x + v1.y * v2.y) + (v1.z * v2.z + v1.w * v2.w);
}
inline float UT_Vector4_Dot_Glm(const Vector4& v1, const Vector4& v2)
{
	return glm::dot(Vector4ToGlmVec4(v1), Vector4ToGlmVec4(v2));
}
inline float UT_Vector4_Dot3(const Vector4& v1, const Vector4& v2)
{
	// somehow the parenthese are crucial to get the same value as SSE or glm version
	return (v1.x * v2.x + v1.y * v2.y) + (v1.z * v2.z);
}
inline float UT_Vector4_Dot3_Glm(const Vector4& v1, const Vector4& v2)
{
	return glm::dot(Vector4ToGlmVec3(v1), Vector4ToGlmVec3(v2));
}
inline float UT_Vector4_Dot2(const Vector4& v1, const Vector4& v2)
{
	// somehow the parenthese are crucial to get the same value as SSE or glm version
	return (v1.x * v2.x + v1.y * v2.y);
}
inline float UT_Vector4_Dot2_Glm(const Vector4& v1, const Vector4& v2)
{
	return glm::dot(Vector4ToGlmVec2(v1), Vector4ToGlmVec2(v2));
}

inline Vector4 UT_Vector4_Cross3(const Vector4& v1, const Vector4& v2)
{
	return Vector4(v1.y*v2.z - v1.z*v2.y, v1.z*v2.x - v1.x*v2.z, v1.x*v2.y - v1.y*v2.x);
}
inline Vector4 UT_Vector4_Cross3_Glm(const Vector4& v1, const Vector4& v2)
{
	return GlmVec3ToVector4(glm::cross(Vector4ToGlmVec3(v1), Vector4ToGlmVec3(v2)));
}


inline Vector4 UT_Vector4_GetNormalized(const Vector4& v1)
{
	float sizeSqr = UT_Vector4_Dot(v1, v1);
	if (sizeSqr < SMALL_NUMBER)
		return Vector4::Zero();
	return UT_Vector4_Div(v1, sqrt(sizeSqr));
}
inline Vector4 UT_Vector4_GetNormalized_Glm(const Vector4& v1)
{
	return GlmVec4ToVector4(glm::normalize(Vector4ToGlmVec4(v1)));
}

inline Vector4 UT_Vector4_GetNormalized3(const Vector4& v1)
{
	float sizeSqr = UT_Vector4_Dot3(v1, v1);
	if (sizeSqr < SMALL_NUMBER)
		return Vector4::Zero();
	return UT_Vector4_Div(v1, sqrt(sizeSqr));
}
inline Vector4 UT_Vector4_GetNormalized3_Glm(const Vector4& v1)
{
	return GlmVec3ToVector4(glm::normalize(Vector4ToGlmVec3(v1)), 0);
}

inline Vector4 UT_Vector4_GetNormalized2(const Vector4& v1)
{
	float sizeSqr = UT_Vector4_Dot2(v1, v1);
	if (sizeSqr < SMALL_NUMBER)
		return Vector4::Zero();
	return UT_Vector4_Div(v1, sqrt(sizeSqr));
}
inline Vector4 UT_Vector4_GetNormalized2_Glm(const Vector4& v1)
{
	return GlmVec2ToVector4(glm::normalize(Vector4ToGlmVec2(v1)), 0);
}




inline Vector4 UT_Vector3_Add(const Vector4& v1, const Vector4& v2)
{
	return Vector4(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, 0);
}