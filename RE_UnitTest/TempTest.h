#pragma once

#include <emmintrin.h>
#include <smmintrin.h>

#include "Math/Vector4.h"


__declspec(align(16)) struct Matrix4
{
public:
	union
	{
		float m[4][4];
		__m128 mVec[4];
		Vector4 mLine[4];
	};

	Matrix4() {};

	Matrix4(const Vector4& line0, const Vector4& line1, const Vector4& line2, const Vector4& line3)
	{
		mLine[0] = line0;
		mLine[1] = line1;
		mLine[2] = line2;
		mLine[3] = line3;
	}
	inline Matrix4& operator+=(const Matrix4& m)
	{
		mVec[0] = VecAdd(mVec[0], m.mVec[0]);
		mVec[1] = VecAdd(mVec[1], m.mVec[1]);
		mVec[2] = VecAdd(mVec[2], m.mVec[2]);
		mVec[3] = VecAdd(mVec[3], m.mVec[3]);
		return *this;
	}
};

#define MakeShuffleMask(x,y,z,w)			(x | (y<<2) | (z<<4) | (w<<6))

// vec(0, 1, 2, 3) -> (vec[x], vec[y], vec[z], vec[w])
#define VecSwizzle(vec, x, y, z, w)			_mm_shuffle_ps(vec, vec, MakeShuffleMask(x,y,z,w))
#define VecSwizzle1(vec, x)					_mm_shuffle_ps(vec, vec, MakeShuffleMask(x,x,x,x))
// special swizzle
#define VecSwizzle_0101(vec)				_mm_movelh_ps(vec, vec)
#define VecSwizzle_2323(vec)				_mm_movehl_ps(vec, vec)
#define VecSwizzle_0022(vec)				_mm_moveldup_ps(vec)
#define VecSwizzle_1133(vec)				_mm_movehdup_ps(vec)

//			0		1		 2		  3
// return (vec1[x], vec1[y], vec2[z], vec2[w])
#define VecShuffle(vec1, vec2, x, y, z, w)	_mm_shuffle_ps(vec1, vec2, MakeShuffleMask(x,y,z,w))
// special shuffle
#define VecShuffle_0101(vec1, vec2)			_mm_movelh_ps(vec1, vec2)
#define VecShuffle_2323(vec1, vec2)			_mm_movehl_ps(vec2, vec1)



// Requires this matrix to be transform matrix, NoScale version requires this matrix be of scale 1
__forceinline Matrix4 GetTransformInverseNoScale(const Matrix4& inM)
{
	Matrix4 r;

	// transpose 3x3, we know m03 = m13 = m23 = 0	
	__m128 t0 = VecShuffle_0101(inM.mVec[0], inM.mVec[1]); // 00, 01, 10, 11
	__m128 t1 = VecShuffle_2323(inM.mVec[0], inM.mVec[1]); // 02, 03, 12, 13
	r.mVec[0] = VecShuffle(t0, inM.mVec[2], 0,2,0,3); // 00, 10, 20, 23(=0)
	r.mVec[1] = VecShuffle(t0, inM.mVec[2], 1,3,1,3); // 01, 11, 21, 23(=0)
	r.mVec[2] = VecShuffle(t1, inM.mVec[2], 0,2,2,3); // 02, 12, 22, 23(=0)

	// last line
	r.mVec[3] =							_mm_mul_ps(r.mVec[0], VecSwizzle1(inM.mVec[3], 0));
	r.mVec[3] = _mm_add_ps(r.mVec[3],	_mm_mul_ps(r.mVec[1], VecSwizzle1(inM.mVec[3], 1)));
	r.mVec[3] = _mm_add_ps(r.mVec[3],	_mm_mul_ps(r.mVec[2], VecSwizzle1(inM.mVec[3], 2)));
	r.mVec[3] = _mm_sub_ps(_mm_setr_ps(0.f, 0.f, 0.f, 1.f), r.mVec[3]);

	return r;
}


#define SMALL_NUMBER		(1.e-8f)

// Requires this matrix to be transform matrix
__forceinline Matrix4 GetTransformInverse(const Matrix4& inM)
{
	Matrix4 r;
	
	// transpose 3x3, we know m03 = m13 = m23 = 0	
	__m128 t0 = VecShuffle_0101(inM.mVec[0], inM.mVec[1]); // 00, 01, 10, 11
	__m128 t1 = VecShuffle_2323(inM.mVec[0], inM.mVec[1]); // 02, 03, 12, 13
	r.mVec[0] = VecShuffle(t0, inM.mVec[2], 0,2,0,3); // 00, 10, 20, 23(=0)
	r.mVec[1] = VecShuffle(t0, inM.mVec[2], 1,3,1,3); // 01, 11, 21, 23(=0)
	r.mVec[2] = VecShuffle(t1, inM.mVec[2], 0,2,2,3); // 02, 12, 22, 23(=0)

	// (SizeSqr(mVec[0]), SizeSqr(mVec[1]), SizeSqr(mVec[2]), 0)
	__m128 sizeSqr;
	sizeSqr =						_mm_mul_ps(r.mVec[0], r.mVec[0]);
	sizeSqr = _mm_add_ps(sizeSqr,	_mm_mul_ps(r.mVec[1], r.mVec[1]));
	sizeSqr = _mm_add_ps(sizeSqr,	_mm_mul_ps(r.mVec[2], r.mVec[2]));

	// optional test to avoid divide by 0
	__m128 one = _mm_set1_ps(1.f);
	// if(sizeSqr < SMALL_NUMBER) sizeSqr = 1;
	__m128 rSizeSqr = _mm_blendv_ps(
		_mm_div_ps(one, sizeSqr),
		one,
		_mm_cmplt_ps(sizeSqr, _mm_set1_ps(SMALL_NUMBER))
		);

	r.mVec[0] = _mm_mul_ps(r.mVec[0], rSizeSqr);
	r.mVec[1] = _mm_mul_ps(r.mVec[1], rSizeSqr);
	r.mVec[2] = _mm_mul_ps(r.mVec[2], rSizeSqr);

	// last line
	r.mVec[3] =							_mm_mul_ps(r.mVec[0], VecSwizzle1(inM.mVec[3], 0));
	r.mVec[3] = _mm_add_ps(r.mVec[3],	_mm_mul_ps(r.mVec[1], VecSwizzle1(inM.mVec[3], 1)));
	r.mVec[3] = _mm_add_ps(r.mVec[3],	_mm_mul_ps(r.mVec[2], VecSwizzle1(inM.mVec[3], 2)));
	r.mVec[3] = _mm_sub_ps(_mm_setr_ps(0.f, 0.f, 0.f, 1.f), r.mVec[3]);

	return r;
}

#if 0
// for row major matrix
// we use __m128 to represent 2x2 matrix as A = | A0  A1 |
//                                              | A2  A3 |
// 2x2 row major Matrix multiply A*B
__forceinline __m128 Mat2Mul(__m128 vec1, __m128 vec2)
{
	return 
		_mm_add_ps(	_mm_mul_ps(						vec1, VecSwizzle(vec2, 0,3,0,3)),
					_mm_mul_ps(VecSwizzle(vec1, 1,0,3,2), VecSwizzle(vec2, 2,1,2,1)));
}
// 2x2 row major Matrix adjugate multiply adj(A)*B
__forceinline __m128 Mat2AdjMul(__m128 vec1, __m128 vec2)
{
	return
		_mm_sub_ps(	_mm_mul_ps(VecSwizzle(vec1, 3,3,0,0), vec2),
					_mm_mul_ps(VecSwizzle(vec1, 1,1,2,2), VecSwizzle(vec2, 2,3,0,1)));

}
// 2x2 row major Matrix multiply adjugate A*adj(B)
__forceinline __m128 Mat2MulAdj(__m128 vec1, __m128 vec2)
{
	return
		_mm_sub_ps(	_mm_mul_ps(						vec1, VecSwizzle(vec2, 3,0,3,0)),
					_mm_mul_ps(VecSwizzle(vec1, 1,0,3,2), VecSwizzle(vec2, 2,1,2,1)));
}

// Inverse function is the same no matter column major or row major
// this version treats it as row major
__forceinline Matrix4 GetInverse(const Matrix4& inM)
{
	// use block matrix method
	// A is a matrix, then i(A) or iA means inverse of A, A# (or A_ in code) means adjugate of A, |A| is determinant, tr(A) is trace
	// some properties: iA = A# / |A|, (AB)# = B#A#, (cA)# = c^(n-1)A#, |AB| = |A||B|, |cA| = c^n|A|, tr(AB) = tr(BA)
	// for 2x2 matrix: (A#)# = A, (cA)# = cA#, |-A| = |A|, |A+B| = |A| + |B| + tr(A#B), |A| = A0*A3 - A1*A2
	// M is 4x4 matrix, we divide into 4 2x2 matrices
	// let M = | A  B |, then iM = | i(A-BiDC)        -iABi(D-CiAB) | = 1/|M| * | (|D|A - B(D#C))#    (|B|C - D(A#B)#)# |
	//         | C  D |            | -iDCi(A-BiDC)    i(D-CiAB)     |           | (|C|B - A(D#C)#)#   (|A|D - C(A#B))#  |
	// we have property: |M| = |A|*|D-CiAB| = |D|*|A-BiDC| = |AD-BC| = |A||D| + |B||C| - tr((A#B)(D#C))
	// we use __m128 to represent sub 2x2 column major matrix as A = | A0  A1 |
	//                                                               | A2  A3 |

	// sub matrices
	__m128 A = VecShuffle_0101(inM.mVec[0], inM.mVec[1]);
	__m128 B = VecShuffle_2323(inM.mVec[0], inM.mVec[1]);
	__m128 C = VecShuffle_0101(inM.mVec[2], inM.mVec[3]);
	__m128 D = VecShuffle_2323(inM.mVec[2], inM.mVec[3]);

	__m128 detA = _mm_set1_ps(inM.m[0][0] * inM.m[1][1] - inM.m[0][1] * inM.m[1][0]);
	__m128 detB = _mm_set1_ps(inM.m[0][2] * inM.m[1][3] - inM.m[0][3] * inM.m[1][2]);
	__m128 detC = _mm_set1_ps(inM.m[2][0] * inM.m[3][1] - inM.m[2][1] * inM.m[3][0]);
	__m128 detD = _mm_set1_ps(inM.m[2][2] * inM.m[3][3] - inM.m[2][3] * inM.m[3][2]);

#if 0 // for determinant, float version is faster
	// determinant as (|A| |B| |C| |D|)
	__m128 detSub = _mm_sub_ps(
		_mm_mul_ps(VecShuffle(inM.mVec[0], inM.mVec[2], 0,2,0,2), VecShuffle(inM.mVec[1], inM.mVec[3], 1,3,1,3)),
		_mm_mul_ps(VecShuffle(inM.mVec[0], inM.mVec[2], 1,3,1,3), VecShuffle(inM.mVec[1], inM.mVec[3], 0,2,0,2))
	);
	__m128 detA = VecSwizzle1(detSub, 0);
	__m128 detB = VecSwizzle1(detSub, 1);
	__m128 detC = VecSwizzle1(detSub, 2);
	__m128 detD = VecSwizzle1(detSub, 3);
#endif

	// let iM = 1/|M| * | X  Y |
	//                  | Z  W |

	// D#C
	__m128 D_C = Mat2AdjMul(D, C);
	// A#B
	__m128 A_B = Mat2AdjMul(A, B);
	// X# = |D|A - B(D#C)
	__m128 X_ = _mm_sub_ps(_mm_mul_ps(detD, A), Mat2Mul(B, D_C));
	// W# = |A|D - C(A#B)
	__m128 W_ = _mm_sub_ps(_mm_mul_ps(detA, D), Mat2Mul(C, A_B));

	// |M| = |A|*|D| + ... (continue later)
	__m128 detM = _mm_mul_ps(detA, detD);

	// Y# = |B|C - D(A#B)#
	__m128 Y_ = _mm_sub_ps(_mm_mul_ps(detB, C), Mat2MulAdj(D, A_B));
	// Z# = |C|B - A(D#C)#
	__m128 Z_ = _mm_sub_ps(_mm_mul_ps(detC, B), Mat2MulAdj(A, D_C));

	// |M| = |A|*|D| + |B|*|C| ... (continue later)
	detM = _mm_add_ps(detM, _mm_mul_ps(detB, detC));

	// tr((A#B)(D#C))
	__m128 tr = _mm_mul_ps(A_B, VecSwizzle(D_C, 0,2,1,3));
	tr = _mm_hadd_ps(tr, tr);
	tr = _mm_hadd_ps(tr, tr);
	// |M| = |A|*|D| + |B|*|C| - tr((A#B)(D#C)
	detM = _mm_sub_ps(detM, tr);

	const __m128 adjSignMask = _mm_castsi128_ps(_mm_setr_epi32(0x00000000, 0x80000000, 0x80000000, 0x00000000));
	// (1/|M|, -1/|M|, -1/|M|, 1/|M|)
	__m128 rDetM = _mm_xor_ps(_mm_div_ps(_mm_set1_ps(1.f), detM), adjSignMask);

	X_ = _mm_mul_ps(X_, rDetM);
	Y_ = _mm_mul_ps(Y_, rDetM);
	Z_ = _mm_mul_ps(Z_, rDetM);
	W_ = _mm_mul_ps(W_, rDetM);

	Matrix4 r;

	// apply adjugate and store, here we combine adjugate shuffle and store shuffle
	// btw adjuagate fuction: Adj(Vec) = VecXor(VecSwizzle(Vec, 3,1,2,0), adjSignMask)
	r.mVec[0] = VecShuffle(X_, Y_, 3,1,3,1);
	r.mVec[1] = VecShuffle(X_, Y_, 2,0,2,0);
	r.mVec[2] = VecShuffle(Z_, W_, 3,1,3,1);
	r.mVec[3] = VecShuffle(Z_, W_, 2,0,2,0);

	return r;
}

#else 
// for column major matrix
// we use __m128 to represent 2x2 matrix as A = | A0  A2 |
//                                              | A1  A3 |
// 2x2 column major Matrix multiply A*B
__forceinline __m128 Mat2Mul(__m128 vec1, __m128 vec2)
{
	return 
		_mm_add_ps(	_mm_mul_ps(						vec1, VecSwizzle(vec2, 0,0,3,3)),
					_mm_mul_ps(VecSwizzle(vec1, 2,3,0,1), VecSwizzle(vec2, 1,1,2,2)));
}
// 2x2 column major Matrix adjugate multiply adj(A)*B
__forceinline __m128 Mat2AdjMul(__m128 vec1, __m128 vec2)
{
	return
		_mm_sub_ps(	_mm_mul_ps(VecSwizzle(vec1, 3,0,3,0), vec2),
					_mm_mul_ps(VecSwizzle(vec1, 2,1,2,1), VecSwizzle(vec2, 1,0,3,2)));

}
// 2x2 column major Matrix multiply adjugate A*adj(B)
__forceinline __m128 Mat2MulAdj(__m128 vec1, __m128 vec2)
{
	return
		_mm_sub_ps(	_mm_mul_ps(						vec1, VecSwizzle(vec2, 3,3,0,0)),
					_mm_mul_ps(VecSwizzle(vec1, 2,3,0,1), VecSwizzle(vec2, 1,1,2,2)));
}

// Inverse function is the same no matter column major or row major
// this version treats it as column major
__forceinline Matrix4 GetInverse(const Matrix4& inM)
{
	// use block matrix method
	// A is a matrix, then i(A) or iA means inverse of A, A# (or A_ in code) means adjugate of A, |A| is determinant, tr(A) is trace
	// some properties: iA = A# / |A|, (AB)# = B#A#, (cA)# = c^(n-1)A#, |AB| = |A||B|, |cA| = c^n|A|, tr(AB) = tr(BA)
	// for 2x2 matrix: (A#)# = A, (cA)# = cA#, |-A| = |A|, |A+B| = |A| + |B| + tr(A#B), |A| = A0*A3 - A1*A2
	// M is 4x4 matrix, we divide into 4 2x2 matrices
	// let M = | A  B |, then iM = | i(A-BiDC)        -iABi(D-CiAB) | = 1/|M| * | (|D|A - B(D#C))#    (|B|C - D(A#B)#)# |
	//         | C  D |            | -iDCi(A-BiDC)    i(D-CiAB)     |           | (|C|B - A(D#C)#)#   (|A|D - C(A#B))#  |
	// we have property: |M| = |A|*|D-CiAB| = |D|*|A-BiDC| = |AD-BC| = |A||D| + |B||C| - tr((A#B)(D#C))
	// we use __m128 to represent sub 2x2 column major matrix as A = | A0  A2 |
	//                                                               | A1  A3 |
				
	// sub matrices
	__m128 A = VecShuffle_0101(inM.mVec[0], inM.mVec[1]);
	__m128 C = VecShuffle_2323(inM.mVec[0], inM.mVec[1]);
	__m128 B = VecShuffle_0101(inM.mVec[2], inM.mVec[3]);
	__m128 D = VecShuffle_2323(inM.mVec[2], inM.mVec[3]);

	__m128 detA = _mm_set1_ps(inM.m[0][0] * inM.m[1][1] - inM.m[0][1] * inM.m[1][0]);
	__m128 detC = _mm_set1_ps(inM.m[0][2] * inM.m[1][3] - inM.m[0][3] * inM.m[1][2]);
	__m128 detB = _mm_set1_ps(inM.m[2][0] * inM.m[3][1] - inM.m[2][1] * inM.m[3][0]);
	__m128 detD = _mm_set1_ps(inM.m[2][2] * inM.m[3][3] - inM.m[2][3] * inM.m[3][2]);

#if 0 // for determinant, float version is faster
	// determinant as (|A| |C| |B| |D|)
	__m128 detSub = _mm_sub_ps(
		_mm_mul_ps(VecShuffle(inM.mVec[0], inM.mVec[2], 0,2,0,2), VecShuffle(inM.mVec[1], inM.mVec[3], 1,3,1,3)),
		_mm_mul_ps(VecShuffle(inM.mVec[0], inM.mVec[2], 1,3,1,3), VecShuffle(inM.mVec[1], inM.mVec[3], 0,2,0,2))
		);
	__m128 detA = VecSwizzle1(detSub, 0);
	__m128 detC = VecSwizzle1(detSub, 1);
	__m128 detB = VecSwizzle1(detSub, 2);
	__m128 detD = VecSwizzle1(detSub, 3);
#endif

	// let iM = 1/|M| * | X  Y |
	//                  | Z  W |

	// D#C
	__m128 D_C = Mat2AdjMul(D, C);
	// A#B
	__m128 A_B = Mat2AdjMul(A, B);
	// X# = |D|A - B(D#C)
	__m128 X_ = _mm_sub_ps(_mm_mul_ps(detD, A), Mat2Mul(B, D_C));
	// W# = |A|D - C(A#B)
	__m128 W_ = _mm_sub_ps(_mm_mul_ps(detA, D), Mat2Mul(C, A_B));

	// |M| = |A|*|D| + ... (continue later)
	__m128 detM = _mm_mul_ps(detA, detD);

	// Y# = |B|C - D(A#B)#
	__m128 Y_ = _mm_sub_ps(_mm_mul_ps(detB, C), Mat2MulAdj(D, A_B));
	// Z# = |C|B - A(D#C)#
	__m128 Z_ = _mm_sub_ps(_mm_mul_ps(detC, B), Mat2MulAdj(A, D_C));

	// |M| = |A|*|D| + |B|*|C| ... (continue later)
	detM = _mm_add_ps(detM, _mm_mul_ps(detB, detC));

	// tr((A#B)(D#C))
	__m128 tr = _mm_mul_ps(A_B, VecSwizzle(D_C, 0,2,1,3));
	tr = _mm_hadd_ps(tr, tr);
	tr = _mm_hadd_ps(tr, tr);
	// |M| = |A|*|D| + |B|*|C| - tr((A#B)(D#C))
	detM = _mm_sub_ps(detM, tr);

	const __m128 adjSignMask = _mm_castsi128_ps(_mm_setr_epi32(0x00000000, 0x80000000, 0x80000000, 0x00000000));
	// (1/|M|, -1/|M|, -1/|M|, 1/|M|)
	__m128 rDetM = _mm_xor_ps(_mm_div_ps(_mm_set1_ps(1.f), detM), adjSignMask);

	X_ = _mm_mul_ps(X_, rDetM);
	Y_ = _mm_mul_ps(Y_, rDetM);
	Z_ = _mm_mul_ps(Z_, rDetM);
	W_ = _mm_mul_ps(W_, rDetM);

	Matrix4 r;

	// apply adjugate and store, here we combine adjugate shuffle and store shuffle
	// btw adjuagate fuction: Adj(Vec) = VecXor(VecSwizzle(Vec, 3,1,2,0), adjSignMask)
	r.mVec[0] = VecShuffle(X_, Z_, 3,1,3,1);
	r.mVec[1] = VecShuffle(X_, Z_, 2,0,2,0);
	r.mVec[2] = VecShuffle(Y_, W_, 3,1,3,1);
	r.mVec[3] = VecShuffle(Y_, W_, 2,0,2,0);

	return r;
}
#endif


#undef SMALL_NUMBER
#undef MakeShuffleMask
#undef VecSwizzle
#undef VecSwizzle1
#undef VecSwizzle_0101
#undef VecSwizzle_2323
#undef VecSwizzle_0022
#undef VecSwizzle_1133
#undef VecShuffle
#undef VecShuffle_0101
#undef VecShuffle_2323