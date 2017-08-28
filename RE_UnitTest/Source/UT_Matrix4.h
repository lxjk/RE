#pragma once

#include "Math/Matrix4.h"
#include "UT_Vector4.h"

//#define Matrix4ToGlmMat4(value) glm::mat4(\
//	Vector4ToGlmVec4(value.mCol[0]),\
//	Vector4ToGlmVec4(value.mCol[1]),\
//	Vector4ToGlmVec4(value.mCol[2]),\
//	Vector4ToGlmVec4(value.mCol[3]))

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
	r.mCol[0] = UT_Matrix4_Mul_Vector4(m1, m2.mCol[0]);
	r.mCol[1] = UT_Matrix4_Mul_Vector4(m1, m2.mCol[1]);
	r.mCol[2] = UT_Matrix4_Mul_Vector4(m1, m2.mCol[2]);
	r.mCol[3] = UT_Matrix4_Mul_Vector4(m1, m2.mCol[3]);
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

__forceinline Matrix4 UT_Matrix4_GetInverse_UE4(const Matrix4& m1)
{
	Matrix4 r;
	float det[4];
	Matrix4 t;

	t.m[0][0] = m1.m[2][2] * m1.m[3][3] - m1.m[2][3] * m1.m[3][2];
	t.m[0][1] = m1.m[1][2] * m1.m[3][3] - m1.m[1][3] * m1.m[3][2];
	t.m[0][2] = m1.m[1][2] * m1.m[2][3] - m1.m[1][3] * m1.m[2][2];

	t.m[1][0] = m1.m[2][2] * m1.m[3][3] - m1.m[2][3] * m1.m[3][2];
	t.m[1][1] = m1.m[0][2] * m1.m[3][3] - m1.m[0][3] * m1.m[3][2];
	t.m[1][2] = m1.m[0][2] * m1.m[2][3] - m1.m[0][3] * m1.m[2][2];

	t.m[2][0] = m1.m[1][2] * m1.m[3][3] - m1.m[1][3] * m1.m[3][2];
	t.m[2][1] = m1.m[0][2] * m1.m[3][3] - m1.m[0][3] * m1.m[3][2];
	t.m[2][2] = m1.m[0][2] * m1.m[1][3] - m1.m[0][3] * m1.m[1][2];

	t.m[3][0] = m1.m[1][2] * m1.m[2][3] - m1.m[1][3] * m1.m[2][2];
	t.m[3][1] = m1.m[0][2] * m1.m[2][3] - m1.m[0][3] * m1.m[2][2];
	t.m[3][2] = m1.m[0][2] * m1.m[1][3] - m1.m[0][3] * m1.m[1][2];

	det[0] = m1.m[1][1] * t.m[0][0] - m1.m[2][1] * t.m[0][1] + m1.m[3][1] * t.m[0][2];
	det[1] = m1.m[0][1] * t.m[1][0] - m1.m[2][1] * t.m[1][1] + m1.m[3][1] * t.m[1][2];
	det[2] = m1.m[0][1] * t.m[2][0] - m1.m[1][1] * t.m[2][1] + m1.m[3][1] * t.m[2][2];
	det[3] = m1.m[0][1] * t.m[3][0] - m1.m[1][1] * t.m[3][1] + m1.m[2][1] * t.m[3][2];

	float Determinant = m1.m[0][0] * det[0] - m1.m[1][0] * det[1] + m1.m[2][0] * det[2] - m1.m[3][0] * det[3];
	const float	RDet = 1.0f / Determinant;

	r.m[0][0] = RDet * det[0];
	r.m[0][1] = -RDet * det[1];
	r.m[0][2] = RDet * det[2];
	r.m[0][3] = -RDet * det[3];
	r.m[1][0] = -RDet * (m1.m[1][0] * t.m[0][0] - m1.m[2][0] * t.m[0][1] + m1.m[3][0] * t.m[0][2]);
	r.m[1][1] = RDet * (m1.m[0][0] * t.m[1][0] - m1.m[2][0] * t.m[1][1] + m1.m[3][0] * t.m[1][2]);
	r.m[1][2] = -RDet * (m1.m[0][0] * t.m[2][0] - m1.m[1][0] * t.m[2][1] + m1.m[3][0] * t.m[2][2]);
	r.m[1][3] = RDet * (m1.m[0][0] * t.m[3][0] - m1.m[1][0] * t.m[3][1] + m1.m[2][0] * t.m[3][2]);
	r.m[2][0] = RDet * (
		m1.m[1][0] * (m1.m[2][1] * m1.m[3][3] - m1.m[2][3] * m1.m[3][1]) -
		m1.m[2][0] * (m1.m[1][1] * m1.m[3][3] - m1.m[1][3] * m1.m[3][1]) +
		m1.m[3][0] * (m1.m[1][1] * m1.m[2][3] - m1.m[1][3] * m1.m[2][1])
		);
	r.m[2][1] = -RDet * (
		m1.m[0][0] * (m1.m[2][1] * m1.m[3][3] - m1.m[2][3] * m1.m[3][1]) -
		m1.m[2][0] * (m1.m[0][1] * m1.m[3][3] - m1.m[0][3] * m1.m[3][1]) +
		m1.m[3][0] * (m1.m[0][1] * m1.m[2][3] - m1.m[0][3] * m1.m[2][1])
		);
	r.m[2][2] = RDet * (
		m1.m[0][0] * (m1.m[1][1] * m1.m[3][3] - m1.m[1][3] * m1.m[3][1]) -
		m1.m[1][0] * (m1.m[0][1] * m1.m[3][3] - m1.m[0][3] * m1.m[3][1]) +
		m1.m[3][0] * (m1.m[0][1] * m1.m[1][3] - m1.m[0][3] * m1.m[1][1])
		);
	r.m[2][3] = -RDet * (
		m1.m[0][0] * (m1.m[1][1] * m1.m[2][3] - m1.m[1][3] * m1.m[2][1]) -
		m1.m[1][0] * (m1.m[0][1] * m1.m[2][3] - m1.m[0][3] * m1.m[2][1]) +
		m1.m[2][0] * (m1.m[0][1] * m1.m[1][3] - m1.m[0][3] * m1.m[1][1])
		);
	r.m[3][0] = -RDet * (
		m1.m[1][0] * (m1.m[2][1] * m1.m[3][2] - m1.m[2][2] * m1.m[3][1]) -
		m1.m[2][0] * (m1.m[1][1] * m1.m[3][2] - m1.m[1][2] * m1.m[3][1]) +
		m1.m[3][0] * (m1.m[1][1] * m1.m[2][2] - m1.m[1][2] * m1.m[2][1])
		);
	r.m[3][1] = RDet * (
		m1.m[0][0] * (m1.m[2][1] * m1.m[3][2] - m1.m[2][2] * m1.m[3][1]) -
		m1.m[2][0] * (m1.m[0][1] * m1.m[3][2] - m1.m[0][2] * m1.m[3][1]) +
		m1.m[3][0] * (m1.m[0][1] * m1.m[2][2] - m1.m[0][2] * m1.m[2][1])
		);
	r.m[3][2] = -RDet * (
		m1.m[0][0] * (m1.m[1][1] * m1.m[3][2] - m1.m[1][2] * m1.m[3][1]) -
		m1.m[1][0] * (m1.m[0][1] * m1.m[3][2] - m1.m[0][2] * m1.m[3][1]) +
		m1.m[3][0] * (m1.m[0][1] * m1.m[1][2] - m1.m[0][2] * m1.m[1][1])
		);
	r.m[3][3] = RDet * (
		m1.m[0][0] * (m1.m[1][1] * m1.m[2][2] - m1.m[1][2] * m1.m[2][1]) -
		m1.m[1][0] * (m1.m[0][1] * m1.m[2][2] - m1.m[0][2] * m1.m[2][1]) +
		m1.m[2][0] * (m1.m[0][1] * m1.m[1][2] - m1.m[0][2] * m1.m[1][1])
		);

	return r;
}

__forceinline Matrix4 UT_Matrix4_GetInverse_Intel(const Matrix4& m1)
{
	Matrix4 r;

	const unsigned int _Sign_PNNP[4] = { 0x00000000, 0x80000000, 0x80000000, 0x00000000 };

	// Load the full matrix into registers
	__m128 _L1 = m1.mVec[0];
	__m128 _L2 = m1.mVec[1];
	__m128 _L3 = m1.mVec[2];
	__m128 _L4 = m1.mVec[3];

	// The inverse is calculated using "Divide and Conquer" technique. The
	// original matrix is divide into four 2x2 sub-matrices. Since each
	// register holds four matrix element, the smaller matrices are
	// represented as a registers. Hence we get a better locality of the
	// calculations.

	__m128 A, B, C, D; // the four sub-matrices

	A = _mm_movelh_ps(_L1, _L2);
	B = _mm_movehl_ps(_L2, _L1);
	C = _mm_movelh_ps(_L3, _L4);
	D = _mm_movehl_ps(_L4, _L3);

	__m128 iA, iB, iC, iD,                 // partial inverse of the sub-matrices
		DC, AB;
	__m128 dA, dB, dC, dD;                 // determinant of the sub-matrices
	__m128 det, d, d1, d2;
	__m128 rd;                             // reciprocal of the determinant

										   //  AB = A# * B
	AB = _mm_mul_ps(_mm_shuffle_ps(A, A, 0x0F), B);
	AB = _mm_sub_ps(AB, _mm_mul_ps(_mm_shuffle_ps(A, A, 0xA5), _mm_shuffle_ps(B, B, 0x4E)));
	//  DC = D# * C
	DC = _mm_mul_ps(_mm_shuffle_ps(D, D, 0x0F), C);
	DC = _mm_sub_ps(DC, _mm_mul_ps(_mm_shuffle_ps(D, D, 0xA5), _mm_shuffle_ps(C, C, 0x4E)));

	//  dA = |A|
	dA = _mm_mul_ps(_mm_shuffle_ps(A, A, 0x5F), A);
	dA = _mm_sub_ss(dA, _mm_movehl_ps(dA, dA));
	//  dB = |B|
	dB = _mm_mul_ps(_mm_shuffle_ps(B, B, 0x5F), B);
	dB = _mm_sub_ss(dB, _mm_movehl_ps(dB, dB));

	//  dC = |C|
	dC = _mm_mul_ps(_mm_shuffle_ps(C, C, 0x5F), C);
	dC = _mm_sub_ss(dC, _mm_movehl_ps(dC, dC));
	//  dD = |D|
	dD = _mm_mul_ps(_mm_shuffle_ps(D, D, 0x5F), D);
	dD = _mm_sub_ss(dD, _mm_movehl_ps(dD, dD));

	//  d = trace(AB*DC) = trace(A#*B*D#*C)
	d = _mm_mul_ps(_mm_shuffle_ps(DC, DC, 0xD8), AB);

	//  iD = C*A#*B
	iD = _mm_mul_ps(_mm_shuffle_ps(C, C, 0xA0), _mm_movelh_ps(AB, AB));
	iD = _mm_add_ps(iD, _mm_mul_ps(_mm_shuffle_ps(C, C, 0xF5), _mm_movehl_ps(AB, AB)));
	//  iA = B*D#*C
	iA = _mm_mul_ps(_mm_shuffle_ps(B, B, 0xA0), _mm_movelh_ps(DC, DC));
	iA = _mm_add_ps(iA, _mm_mul_ps(_mm_shuffle_ps(B, B, 0xF5), _mm_movehl_ps(DC, DC)));

	//  d = trace(AB*DC) = trace(A#*B*D#*C) [continue]
	d = _mm_add_ps(d, _mm_movehl_ps(d, d));
	d = _mm_add_ss(d, _mm_shuffle_ps(d, d, 1));
	d1 = _mm_mul_ss(dA, dD);
	d2 = _mm_mul_ss(dB, dC);

	//  iD = D*|A| - C*A#*B
	iD = _mm_sub_ps(_mm_mul_ps(D, _mm_shuffle_ps(dA, dA, 0)), iD);

	//  iA = A*|D| - B*D#*C;
	iA = _mm_sub_ps(_mm_mul_ps(A, _mm_shuffle_ps(dD, dD, 0)), iA);

	//  det = |A|*|D| + |B|*|C| - trace(A#*B*D#*C)
	det = _mm_sub_ss(_mm_add_ss(d1, d2), d);
	rd = _mm_div_ss(_mm_set_ss(1.0f), det);


	//  iB = D * (A#B)# = D*B#*A
	iB = _mm_mul_ps(D, _mm_shuffle_ps(AB, AB, 0x33));
	iB = _mm_sub_ps(iB, _mm_mul_ps(_mm_shuffle_ps(D, D, 0xB1), _mm_shuffle_ps(AB, AB, 0x66)));
	//  iC = A * (D#C)# = A*C#*D
	iC = _mm_mul_ps(A, _mm_shuffle_ps(DC, DC, 0x33));
	iC = _mm_sub_ps(iC, _mm_mul_ps(_mm_shuffle_ps(A, A, 0xB1), _mm_shuffle_ps(DC, DC, 0x66)));

	rd = _mm_shuffle_ps(rd, rd, 0);
	rd = _mm_xor_ps(rd, _mm_load_ps((float*)_Sign_PNNP));

	//  iB = C*|B| - D*B#*A
	iB = _mm_sub_ps(_mm_mul_ps(C, _mm_shuffle_ps(dB, dB, 0)), iB);

	//  iC = B*|C| - A*C#*D;
	iC = _mm_sub_ps(_mm_mul_ps(B, _mm_shuffle_ps(dC, dC, 0)), iC);

	//  iX = iX / det
	iA = _mm_mul_ps(rd, iA);
	iB = _mm_mul_ps(rd, iB);
	iC = _mm_mul_ps(rd, iC);
	iD = _mm_mul_ps(rd, iD);

	r.mVec[0] = _mm_shuffle_ps(iA, iB, 0x77);
	r.mVec[1] = _mm_shuffle_ps(iA, iB, 0x22);
	r.mVec[2] = _mm_shuffle_ps(iC, iD, 0x77);
	r.mVec[3] = _mm_shuffle_ps(iC, iD, 0x22);

	return r;
}

__forceinline Matrix4 UT_Matrix4_GetInverse_DirectX(const Matrix4& m1)
{
	Matrix4 r;

	//XMMATRIX MT = XMMatrixTranspose(M);
	Vec128 V00 = VecSwizzleMask(m1.mVec[2], _MM_SHUFFLE(1, 1, 0, 0));
	Vec128 V10 = VecSwizzleMask(m1.mVec[3], _MM_SHUFFLE(3, 2, 3, 2));
	Vec128 V01 = VecSwizzleMask(m1.mVec[0], _MM_SHUFFLE(1, 1, 0, 0));
	Vec128 V11 = VecSwizzleMask(m1.mVec[1], _MM_SHUFFLE(3, 2, 3, 2));
	Vec128 V02 = _mm_shuffle_ps(m1.mVec[2], m1.mVec[0], _MM_SHUFFLE(2, 0, 2, 0));
	Vec128 V12 = _mm_shuffle_ps(m1.mVec[3], m1.mVec[1], _MM_SHUFFLE(3, 1, 3, 1));

	Vec128 D0 = _mm_mul_ps(V00, V10);
	Vec128 D1 = _mm_mul_ps(V01, V11);
	Vec128 D2 = _mm_mul_ps(V02, V12);

	V00 = VecSwizzleMask(m1.mVec[2], _MM_SHUFFLE(3, 2, 3, 2));
	V10 = VecSwizzleMask(m1.mVec[3], _MM_SHUFFLE(1, 1, 0, 0));
	V01 = VecSwizzleMask(m1.mVec[0], _MM_SHUFFLE(3, 2, 3, 2));
	V11 = VecSwizzleMask(m1.mVec[1], _MM_SHUFFLE(1, 1, 0, 0));
	V02 = _mm_shuffle_ps(m1.mVec[2], m1.mVec[0], _MM_SHUFFLE(3, 1, 3, 1));
	V12 = _mm_shuffle_ps(m1.mVec[3], m1.mVec[1], _MM_SHUFFLE(2, 0, 2, 0));

	V00 = _mm_mul_ps(V00, V10);
	V01 = _mm_mul_ps(V01, V11);
	V02 = _mm_mul_ps(V02, V12);
	D0 = _mm_sub_ps(D0, V00);
	D1 = _mm_sub_ps(D1, V01);
	D2 = _mm_sub_ps(D2, V02);
	// V11 = D0Y,D0W,D2Y,D2Y
	V11 = _mm_shuffle_ps(D0, D2, _MM_SHUFFLE(1, 1, 3, 1));
	V00 = VecSwizzleMask(m1.mVec[1], _MM_SHUFFLE(1, 0, 2, 1));
	V10 = _mm_shuffle_ps(V11, D0, _MM_SHUFFLE(0, 3, 0, 2));
	V01 = VecSwizzleMask(m1.mVec[0], _MM_SHUFFLE(0, 1, 0, 2));
	V11 = _mm_shuffle_ps(V11, D0, _MM_SHUFFLE(2, 1, 2, 1));
	// V13 = D1Y,D1W,D2W,D2W
	Vec128 V13 = _mm_shuffle_ps(D1, D2, _MM_SHUFFLE(3, 3, 3, 1));
	V02 = VecSwizzleMask(m1.mVec[3], _MM_SHUFFLE(1, 0, 2, 1));
	V12 = _mm_shuffle_ps(V13, D1, _MM_SHUFFLE(0, 3, 0, 2));
	Vec128 V03 = VecSwizzleMask(m1.mVec[2], _MM_SHUFFLE(0, 1, 0, 2));
	V13 = _mm_shuffle_ps(V13, D1, _MM_SHUFFLE(2, 1, 2, 1));

	Vec128 C0 = _mm_mul_ps(V00, V10);
	Vec128 C2 = _mm_mul_ps(V01, V11);
	Vec128 C4 = _mm_mul_ps(V02, V12);
	Vec128 C6 = _mm_mul_ps(V03, V13);

	// V11 = D0X,D0Y,D2X,D2X
	V11 = _mm_shuffle_ps(D0, D2, _MM_SHUFFLE(0, 0, 1, 0));
	V00 = VecSwizzleMask(m1.mVec[1], _MM_SHUFFLE(2, 1, 3, 2));
	V10 = _mm_shuffle_ps(D0, V11, _MM_SHUFFLE(2, 1, 0, 3));
	V01 = VecSwizzleMask(m1.mVec[0], _MM_SHUFFLE(1, 3, 2, 3));
	V11 = _mm_shuffle_ps(D0, V11, _MM_SHUFFLE(0, 2, 1, 2));
	// V13 = D1X,D1Y,D2Z,D2Z
	V13 = _mm_shuffle_ps(D1, D2, _MM_SHUFFLE(2, 2, 1, 0));
	V02 = VecSwizzleMask(m1.mVec[3], _MM_SHUFFLE(2, 1, 3, 2));
	V12 = _mm_shuffle_ps(D1, V13, _MM_SHUFFLE(2, 1, 0, 3));
	V03 = VecSwizzleMask(m1.mVec[2], _MM_SHUFFLE(1, 3, 2, 3));
	V13 = _mm_shuffle_ps(D1, V13, _MM_SHUFFLE(0, 2, 1, 2));

	V00 = _mm_mul_ps(V00, V10);
	V01 = _mm_mul_ps(V01, V11);
	V02 = _mm_mul_ps(V02, V12);
	V03 = _mm_mul_ps(V03, V13);
	C0 = _mm_sub_ps(C0, V00);
	C2 = _mm_sub_ps(C2, V01);
	C4 = _mm_sub_ps(C4, V02);
	C6 = _mm_sub_ps(C6, V03);

	V00 = VecSwizzleMask(m1.mVec[1], _MM_SHUFFLE(0, 3, 0, 3));
	// V10 = D0Z,D0Z,D2X,D2Y
	V10 = _mm_shuffle_ps(D0, D2, _MM_SHUFFLE(1, 0, 2, 2));
	V10 = VecSwizzleMask(V10, _MM_SHUFFLE(0, 2, 3, 0));
	V01 = VecSwizzleMask(m1.mVec[0], _MM_SHUFFLE(2, 0, 3, 1));
	// V11 = D0X,D0W,D2X,D2Y
	V11 = _mm_shuffle_ps(D0, D2, _MM_SHUFFLE(1, 0, 3, 0));
	V11 = VecSwizzleMask(V11, _MM_SHUFFLE(2, 1, 0, 3));
	V02 = VecSwizzleMask(m1.mVec[3], _MM_SHUFFLE(0, 3, 0, 3));
	// V12 = D1Z,D1Z,D2Z,D2W
	V12 = _mm_shuffle_ps(D1, D2, _MM_SHUFFLE(3, 2, 2, 2));
	V12 = VecSwizzleMask(V12, _MM_SHUFFLE(0, 2, 3, 0));
	V03 = VecSwizzleMask(m1.mVec[2], _MM_SHUFFLE(2, 0, 3, 1));
	// V13 = D1X,D1W,D2Z,D2W
	V13 = _mm_shuffle_ps(D1, D2, _MM_SHUFFLE(3, 2, 3, 0));
	V13 = VecSwizzleMask(V13, _MM_SHUFFLE(2, 1, 0, 3));

	V00 = _mm_mul_ps(V00, V10);
	V01 = _mm_mul_ps(V01, V11);
	V02 = _mm_mul_ps(V02, V12);
	V03 = _mm_mul_ps(V03, V13);
	Vec128 C1 = _mm_sub_ps(C0, V00);
	C0 = _mm_add_ps(C0, V00);
	Vec128 C3 = _mm_add_ps(C2, V01);
	C2 = _mm_sub_ps(C2, V01);
	Vec128 C5 = _mm_sub_ps(C4, V02);
	C4 = _mm_add_ps(C4, V02);
	Vec128 C7 = _mm_add_ps(C6, V03);
	C6 = _mm_sub_ps(C6, V03);

	C0 = _mm_shuffle_ps(C0, C1, _MM_SHUFFLE(3, 1, 2, 0));
	C2 = _mm_shuffle_ps(C2, C3, _MM_SHUFFLE(3, 1, 2, 0));
	C4 = _mm_shuffle_ps(C4, C5, _MM_SHUFFLE(3, 1, 2, 0));
	C6 = _mm_shuffle_ps(C6, C7, _MM_SHUFFLE(3, 1, 2, 0));
	C0 = VecSwizzleMask(C0, _MM_SHUFFLE(3, 1, 2, 0));
	C2 = VecSwizzleMask(C2, _MM_SHUFFLE(3, 1, 2, 0));
	C4 = VecSwizzleMask(C4, _MM_SHUFFLE(3, 1, 2, 0));
	C6 = VecSwizzleMask(C6, _MM_SHUFFLE(3, 1, 2, 0));
	// Get the determinate
	Vec128 vTemp = VecDotV(C0, m1.mVec[0]);
	vTemp = _mm_div_ps(VecSet1(1.f), vTemp);
	r.mVec[0] = _mm_mul_ps(C0, vTemp);
	r.mVec[1] = _mm_mul_ps(C2, vTemp);
	r.mVec[2] = _mm_mul_ps(C4, vTemp);
	r.mVec[3] = _mm_mul_ps(C6, vTemp);

	return r;
}

inline Matrix4 UT_Matrix4_GetInverse_Glm(const Matrix4& m1)
{
	return GlmMat4ToMatrix4(glm::inverse(Matrix4ToGlmMat4(m1)));
}

__forceinline Matrix4 UT_Matrix4_GetInverseTransposed3(const Matrix4& m1)
{
	Matrix4 r;

	float Determinant =
		+ m1.m[0][0] * (m1.m[1][1] * m1.m[2][2] - m1.m[1][2] * m1.m[2][1])
		- m1.m[0][1] * (m1.m[1][0] * m1.m[2][2] - m1.m[1][2] * m1.m[2][0])
		+ m1.m[0][2] * (m1.m[1][0] * m1.m[2][1] - m1.m[1][1] * m1.m[2][0]);

	r.m[0][0] = +(m1.m[1][1] * m1.m[2][2] - m1.m[2][1] * m1.m[1][2]);
	r.m[0][1] = -(m1.m[1][0] * m1.m[2][2] - m1.m[2][0] * m1.m[1][2]);
	r.m[0][2] = +(m1.m[1][0] * m1.m[2][1] - m1.m[2][0] * m1.m[1][1]);
	r.m[1][0] = -(m1.m[0][1] * m1.m[2][2] - m1.m[2][1] * m1.m[0][2]);
	r.m[1][1] = +(m1.m[0][0] * m1.m[2][2] - m1.m[2][0] * m1.m[0][2]);
	r.m[1][2] = -(m1.m[0][0] * m1.m[2][1] - m1.m[2][0] * m1.m[0][1]);
	r.m[2][0] = +(m1.m[0][1] * m1.m[1][2] - m1.m[1][1] * m1.m[0][2]);
	r.m[2][1] = -(m1.m[0][0] * m1.m[1][2] - m1.m[1][0] * m1.m[0][2]);
	r.m[2][2] = +(m1.m[0][0] * m1.m[1][1] - m1.m[1][0] * m1.m[0][1]);
	r /= Determinant;

	r.mVec[3] = VecSet(0.f, 0.f, 0.f, 1.f);

	return r;
}

__forceinline float UT_Matrix4_GetDeterminant(const Matrix4& m1)
{
	return	m1.m[0][0] * (
				m1.m[1][1] * (m1.m[2][2] * m1.m[3][3] - m1.m[2][3] * m1.m[3][2]) -
				m1.m[2][1] * (m1.m[1][2] * m1.m[3][3] - m1.m[1][3] * m1.m[3][2]) +
				m1.m[3][1] * (m1.m[1][2] * m1.m[2][3] - m1.m[1][3] * m1.m[2][2])
				) -
			m1.m[1][0] * (
				m1.m[0][1] * (m1.m[2][2] * m1.m[3][3] - m1.m[2][3] * m1.m[3][2]) -
				m1.m[2][1] * (m1.m[0][2] * m1.m[3][3] - m1.m[0][3] * m1.m[3][2]) +
				m1.m[3][1] * (m1.m[0][2] * m1.m[2][3] - m1.m[0][3] * m1.m[2][2])
				) +
			m1.m[2][0] * (
				m1.m[0][1] * (m1.m[1][2] * m1.m[3][3] - m1.m[1][3] * m1.m[3][2]) -
				m1.m[1][1] * (m1.m[0][2] * m1.m[3][3] - m1.m[0][3] * m1.m[3][2]) +
				m1.m[3][1] * (m1.m[0][2] * m1.m[1][3] - m1.m[0][3] * m1.m[1][2])
				) -
			m1.m[3][0] * (
				m1.m[0][1] * (m1.m[1][2] * m1.m[2][3] - m1.m[1][3] * m1.m[2][2]) -
				m1.m[1][1] * (m1.m[0][2] * m1.m[2][3] - m1.m[0][3] * m1.m[2][2]) +
				m1.m[2][1] * (m1.m[0][2] * m1.m[1][3] - m1.m[0][3] * m1.m[1][2])
				);
}