#pragma once

#include "Math/Matrix4.h"
#include "UT_Vector4.h"

//#define Matrix4ToGlmMat4(value) glm::mat4(\
//	Vector4ToGlmVec4(value.mColumns[0]),\
//	Vector4ToGlmVec4(value.mColumns[1]),\
//	Vector4ToGlmVec4(value.mColumns[2]),\
//	Vector4ToGlmVec4(value.mColumns[3]))

//#define GlmMat4ToMatrix4(value) Matrix4(\
//	GlmVec4ToVector4(value[0]),\
//	GlmVec4ToVector4(value[1]),\
//	GlmVec4ToVector4(value[2]),\
//	GlmVec4ToVector4(value[3]))

#define Matrix4ToGlmMat4(value) (*(glm::mat4*)(&value))
#define GlmMat4ToMatrix4(value) (*(Matrix4*)(&value))

inline Matrix4 UT_Matrix4_Add_Glm(const Matrix4& m1, const Matrix4& m2)
{
	glm::mat4 r = Matrix4ToGlmMat4(m1) + Matrix4ToGlmMat4(m2);
	return GlmMat4ToMatrix4(r);
}
inline Vector4 UT_Matrix4_Mul_Vector4(const Matrix4& m1, const Vector4& v2)
{
	Vector4 r;
	r.x = m1.m[0][0] * v2.x + m1.m[1][0] * v2.y + m1.m[2][0] * v2.z + m1.m[3][0] * v2.w;
	r.y = m1.m[0][1] * v2.x + m1.m[1][1] * v2.y + m1.m[2][1] * v2.z + m1.m[3][1] * v2.w;
	r.z = m1.m[0][2] * v2.x + m1.m[1][2] * v2.y + m1.m[2][2] * v2.z + m1.m[3][2] * v2.w;
	r.w = m1.m[0][3] * v2.x + m1.m[1][3] * v2.y + m1.m[2][3] * v2.z + m1.m[3][3] * v2.w;
	return r;
}

inline Vector4 UT_Matrix4_Mul_Vector4_Glm(const Matrix4& m1, const Vector4& v2)
{
	glm::vec4 r = Matrix4ToGlmMat4(m1) * Vector4ToGlmVec4(v2);
	return GlmVec4ToVector4(r);
}

inline Matrix4 UT_Matrix4_Mul_Matrix4(const Matrix4& m1, const Matrix4& m2)
{
	Matrix4 r;
	r.mColumns[0] = UT_Matrix4_Mul_Vector4(m1, m2.mColumns[0]);
	r.mColumns[1] = UT_Matrix4_Mul_Vector4(m1, m2.mColumns[1]);
	r.mColumns[2] = UT_Matrix4_Mul_Vector4(m1, m2.mColumns[2]);
	r.mColumns[3] = UT_Matrix4_Mul_Vector4(m1, m2.mColumns[3]);
	return r;
}

inline Matrix4 UT_Matrix4_Mul_Matrix4_Glm(const Matrix4& m1, const Matrix4& m2)
{
	glm::mat4 r = Matrix4ToGlmMat4(m1) * Matrix4ToGlmMat4(m2);
	return GlmMat4ToMatrix4(r);
}

inline Matrix4 UT_Matrix4_GetTransposed(const Matrix4& m1)
{
	Matrix4 r;
	r.m[0][0] = m1.m[0][0];
	r.m[0][1] = m1.m[1][0];
	r.m[0][2] = m1.m[2][0];
	r.m[0][3] = m1.m[3][0];

	r.m[1][0] = m1.m[0][1];
	r.m[1][1] = m1.m[1][1];
	r.m[1][2] = m1.m[2][1];
	r.m[1][3] = m1.m[3][1];

	r.m[2][0] = m1.m[0][2];
	r.m[2][1] = m1.m[1][2];
	r.m[2][2] = m1.m[2][2];
	r.m[2][3] = m1.m[3][2];

	r.m[3][0] = m1.m[0][3];
	r.m[3][1] = m1.m[1][3];
	r.m[3][2] = m1.m[2][3];
	r.m[3][3] = m1.m[3][3];
	return r;
}

inline Matrix4 UT_Matrix4_GetTransposed_Glm(const Matrix4& m1)
{
	return GlmMat4ToMatrix4(glm::transpose(Matrix4ToGlmMat4(m1)));
}