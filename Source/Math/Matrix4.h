#pragma once

#include "MathUtil.h"

#include "Vector4.h"

struct Matrix4;

// Column major matrix 4x4
__declspec(align(16))
struct Matrix4
{
public:
	union
	{
		float m[4][4];
		Vec128 mVec[4];
		Vector4 mColumns[4];
	};

	Matrix4() {};

	Matrix4(const Vector4& col0, const Vector4& col1, const Vector4& col2, const Vector4& col3)
	{
		mColumns[0] = col0;
		mColumns[1] = col1;
		mColumns[2] = col2;
		mColumns[3] = col3;
	}

	// add
	inline Matrix4 operator+(const Matrix4& m) const
	{
		Matrix4 r;
		r.mVec[0] = VecAdd(mVec[0], m.mVec[0]);
		r.mVec[1] = VecAdd(mVec[1], m.mVec[1]);
		r.mVec[2] = VecAdd(mVec[2], m.mVec[2]);
		r.mVec[3] = VecAdd(mVec[3], m.mVec[3]);
		return r;
	}
	inline Matrix4& operator+=(const Matrix4& m)
	{
		mVec[0] = VecAdd(mVec[0], m.mVec[0]);
		mVec[1] = VecAdd(mVec[1], m.mVec[1]);
		mVec[2] = VecAdd(mVec[2], m.mVec[2]);
		mVec[3] = VecAdd(mVec[3], m.mVec[3]);
		return *this;
	}

	inline Matrix4& operator+=(float f)
	{
		Vec128 t = VecSet1(f);
		mVec[0] = VecAdd(mVec[0], t);
		mVec[1] = VecAdd(mVec[1], t);
		mVec[2] = VecAdd(mVec[2], t);
		mVec[3] = VecAdd(mVec[3], t);
		return *this;
	}

	// sub
	inline Matrix4 operator-(const Matrix4& m) const
	{
		Matrix4 r;
		r.mVec[0] = VecSub(mVec[0], m.mVec[0]);
		r.mVec[1] = VecSub(mVec[1], m.mVec[1]);
		r.mVec[2] = VecSub(mVec[2], m.mVec[2]);
		r.mVec[3] = VecSub(mVec[3], m.mVec[3]);
		return r;
	}
	inline Matrix4& operator-=(const Matrix4& m)
	{
		mVec[0] = VecSub(mVec[0], m.mVec[0]);
		mVec[1] = VecSub(mVec[1], m.mVec[1]);
		mVec[2] = VecSub(mVec[2], m.mVec[2]);
		mVec[3] = VecSub(mVec[3], m.mVec[3]);
		return *this;
	}

	// mul scalar
	inline Matrix4 operator*(float f) const
	{
		Matrix4 r;
		Vec128 t = VecSet1(f);
		r.mVec[0] = VecMul(mVec[0], t);
		r.mVec[1] = VecMul(mVec[1], t);
		r.mVec[2] = VecMul(mVec[2], t);
		r.mVec[3] = VecMul(mVec[3], t);
		return r;
	}
	inline Matrix4& operator*=(float f)
	{
		Vec128 t = VecSet1(f);
		mVec[0] = VecMul(mVec[0], t);
		mVec[1] = VecMul(mVec[1], t);
		mVec[2] = VecMul(mVec[2], t);
		mVec[3] = VecMul(mVec[3], t);
		return *this;
	}

	// div scalar
	inline Matrix4 operator/(float f) const
	{
		Matrix4 r;
		Vec128 t = VecSet1(f);
		r.mVec[0] = VecDiv(mVec[0], t);
		r.mVec[1] = VecDiv(mVec[1], t);
		r.mVec[2] = VecDiv(mVec[2], t);
		r.mVec[3] = VecDiv(mVec[3], t);
		return r;
	}
	inline Matrix4& operator/=(float f)
	{
		Vec128 t = VecSet1(f);
		mVec[0] = VecDiv(mVec[0], t);
		mVec[1] = VecDiv(mVec[1], t);
		mVec[2] = VecDiv(mVec[2], t);
		mVec[3] = VecDiv(mVec[3], t);
		return *this;
	}

	// mul vector
	inline Vector4 operator*(const Vector4& v) const
	{
		Vec128 r;
		r =				VecMul(mVec[0], VecSwizzle1(v.mVec,0));
		r = VecAdd(r,	VecMul(mVec[1], VecSwizzle1(v.mVec,1)));
		r = VecAdd(r,	VecMul(mVec[2], VecSwizzle1(v.mVec,2)));
		r = VecAdd(r,	VecMul(mVec[3], VecSwizzle1(v.mVec,3)));
		return r;
	}

	// mul matrix
	inline Matrix4 operator*(const Matrix4& m) const
	{
		Matrix4 r;
		// column 0
		r.mVec[0] =						VecMul(mVec[0], VecSwizzle1(m.mVec[0], 0));
		r.mVec[0] = VecAdd(r.mVec[0],	VecMul(mVec[1], VecSwizzle1(m.mVec[0], 1)));
		r.mVec[0] = VecAdd(r.mVec[0],	VecMul(mVec[2], VecSwizzle1(m.mVec[0], 2)));
		r.mVec[0] = VecAdd(r.mVec[0],	VecMul(mVec[3], VecSwizzle1(m.mVec[0], 3)));
		// column 1
		r.mVec[1] =						VecMul(mVec[0], VecSwizzle1(m.mVec[1], 0));
		r.mVec[1] = VecAdd(r.mVec[1],	VecMul(mVec[1], VecSwizzle1(m.mVec[1], 1)));
		r.mVec[1] = VecAdd(r.mVec[1],	VecMul(mVec[2], VecSwizzle1(m.mVec[1], 2)));
		r.mVec[1] = VecAdd(r.mVec[1],	VecMul(mVec[3], VecSwizzle1(m.mVec[1], 3)));
		// column 2
		r.mVec[2] =						VecMul(mVec[0], VecSwizzle1(m.mVec[2], 0));
		r.mVec[2] = VecAdd(r.mVec[2],	VecMul(mVec[1], VecSwizzle1(m.mVec[2], 1)));
		r.mVec[2] = VecAdd(r.mVec[2],	VecMul(mVec[2], VecSwizzle1(m.mVec[2], 2)));
		r.mVec[2] = VecAdd(r.mVec[2],	VecMul(mVec[3], VecSwizzle1(m.mVec[2], 3)));
		// column 3
		r.mVec[3] =						VecMul(mVec[0], VecSwizzle1(m.mVec[3], 0));
		r.mVec[3] = VecAdd(r.mVec[3],	VecMul(mVec[1], VecSwizzle1(m.mVec[3], 1)));
		r.mVec[3] = VecAdd(r.mVec[3],	VecMul(mVec[2], VecSwizzle1(m.mVec[3], 2)));
		r.mVec[3] = VecAdd(r.mVec[3],	VecMul(mVec[3], VecSwizzle1(m.mVec[3], 3)));
		return r;
	}


	inline Matrix4 GetTransposed() const
	{
		// from _MM_TRANSPOSE4_PS()
		Matrix4 r;

		Vec128 t0 = VecShuffle(mVec[0], mVec[1], 0, 1, 0, 1); // 00, 01, 10, 11
		Vec128 t1 = VecShuffle(mVec[2], mVec[3], 0, 1, 0, 1); // 20, 21, 30, 31
		Vec128 t2 = VecShuffle(mVec[0], mVec[1], 2, 3, 2, 3); // 02, 03, 12, 13
		Vec128 t3 = VecShuffle(mVec[2], mVec[3], 2, 3, 2, 3); // 22, 23, 32, 33

		r.mVec[0] = VecShuffle(t0, t1, 0, 2, 0, 2); // 00, 10, 20, 30
		r.mVec[1] = VecShuffle(t0, t1, 1, 3, 1, 3); // 01, 11, 21, 31
		r.mVec[2] = VecShuffle(t2, t3, 0, 2, 0, 2); // 02, 12, 22, 32
		r.mVec[3] = VecShuffle(t2, t3, 1, 3, 1, 3); // 03, 13, 23, 33

		return r;		
	}

	inline Matrix4 GetInversed() const
	{
		Matrix4 r;
		return r;
	}
};