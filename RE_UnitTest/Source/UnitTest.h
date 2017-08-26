#pragma once

#include <stdlib.h>
#include <functional>

#include "Math/MathUtil.h"
#include "UT_Vector4.h"
#include "UT_Matrix4.h"

// float bit
// 31 30-23(8) 22-0(23)
//sign exp     frac
IntFloatUnion FloatSpecials[] =
{
	0xFFFFFFFF, // - QNaN
	0x7FFFFFFF, // + QNaN
	0xFF800001, // - SNaN
	0x7F800001, // + SNaN
	0xFF800000, // - Infinity
	0x7F800000, // + Infinity
	0xFF7FFFFF, // - Max
	0x7F7FFFFF, // + Max
	0xBF800000, // - 1.0
	0x3F800000, // + 1.0
	0x80800000, // - Normalized Min
	0x00800000, // + Normalized Min
	0x807FFFFF, // - Denormalized Max
	0x007FFFFF, // + Denormalized Max
	0x80000001, // - Min
	0x00000001, // + Min
	0x80000000, // - 0.0
	0x00000000, // + 0.0
};
int FloatSpecialNum = _countof(FloatSpecials);


#define IsFloatSpecial(i) ((i & 0x7F800000) == 0x7F800000)
#define IsNaN(i) (IsFloatSpecial(i) && (i & 0x007FFFFF) > 0)
#define IsQNaN(i) (IsFloatSpecial(i) && (i & 0x00400000) == 0x00400000)
#define IsSNaN(i) (IsFloatSpecial(i) && (i & 0x00400000) == 0)
#define max(a, b) (a > b ? a : b)



template<typename T>
void PrintValue(T v);

template<>
inline void PrintValue(float v)
{
	IntFloatUnion u;
	u.f = v;
	printf("\t[%08x, %+.04e]", u.i, u.f);
	printf("\n");
}

template<>
inline void PrintValue(Vector4 v)
{
	for (int i = 0; i < 4; ++i)
	{
		IntFloatUnion u;
		u.f = v.m[i];
		printf("\t[%08x, %+.04e]", u.i, u.f);
	}
	printf("\n");
}

template<>
inline void PrintValue(Matrix4 v)
{
	for (int i = 0; i < 4; ++i)
	{
		PrintValue(v.mColumns[i]);
	}
}

template<typename T>
int CheckEquals(T v1, T v2);

// 0 same, 
// 1 same special value differnt sign bit,
// 2 different nan,
// 3 within threshould, 
// 4 different special value, 
// 5 differnt valid float
template<>
inline int CheckEquals(float f1, float f2)
{
	IntFloatUnion u1, u2;
	u1.f = f1;
	u2.f = f2;

	if (u1.i == u2.i)
		return 0;

	// this is for +0.0 and -0.0
	if (f1 == f2)
		return 1;

	if (IsFloatSpecial(u1.i) || IsFloatSpecial(u2.i))
	{
		// if same type of NaN, let it pass
		if ((IsQNaN(u1.i) && IsQNaN(u2.i)) ||
			(IsSNaN(u1.i) && IsSNaN(u2.i)))
			return 0;

		// remove sign bit and try again
		unsigned __int32 i1 = u1.i & 0x7FFFFFFF;
		unsigned __int32 i2 = u2.i & 0x7FFFFFFF;
		if (i1 == i2 ||
			(IsQNaN(i1) && IsQNaN(i2)) ||
			(IsSNaN(i1) && IsSNaN(i2)))
			return 1;

		if (IsNaN(i1) && IsNaN(i2))
			return 2;

		if (f1 == 0 || f2 == 0)
			return 2;

		//PrintValue(f1);
		//PrintValue(f2);

		return 4;
	}

	if (abs(u1.i - u2.i) < 2)
		return 3;

	if (abs(f1 - f2) < SMALL_NUMBER)
		return 3;

	return 5;
}

template<>
inline int CheckEquals(Vector4 v1, Vector4 v2)
{
	return 
		max(CheckEquals(v1.x, v2.x),
		max(CheckEquals(v1.y, v2.y),
		max(CheckEquals(v1.z, v2.z),
			CheckEquals(v1.w, v2.w))));
}

template<>
inline int CheckEquals(Matrix4 m1, Matrix4 m2)
{
	return 
		max(CheckEquals(m1.mColumns[0], m2.mColumns[0]),
		max(CheckEquals(m1.mColumns[1], m2.mColumns[1]),
		max(CheckEquals(m1.mColumns[2], m2.mColumns[2]),
			CheckEquals(m1.mColumns[3], m2.mColumns[3]))));
}

template<typename T_op1, typename T_op2, typename T_r>
void Check(T_op1 v1, T_op2 v2, T_r r1, T_r r2, T_r r3, int level)
{
	int e2 = CheckEquals(r1, r2);
	if (e2 >level)
	{
		printf("1-2 result: %d\n", e2);
		PrintValue(v1);
		PrintValue(v2);
		PrintValue(r1);
		PrintValue(r2);
		__debugbreak();
	}

	int e3 = CheckEquals(r1, r3);
	if (e3 > level)
	{
		printf("1-3 result: %d\n", e3);
		PrintValue(v1);
		PrintValue(v2);
		PrintValue(r1);
		PrintValue(r3);
		__debugbreak();
	}
}

template<typename T_op, typename T_r>
void Check(T_op v, T_r r1, T_r r2, T_r r3, int level)
{
	int e2 = CheckEquals(r1, r2);
	if (e2 >level)
	{
		printf("1-2 result: %d\n", e2);
		PrintValue(v);
		PrintValue(r1);
		PrintValue(r2);
		__debugbreak();
	}

	int e3 = CheckEquals(r1, r3);
	if (e3 > level)
	{
		printf("1-3 result: %d\n", e3);
		PrintValue(v);
		PrintValue(r1);
		PrintValue(r3);
		__debugbreak();
	}
}

void ExhaustTest()
{
	IntFloatUnion u;

	unsigned __int64 i64;

	unsigned int counter = 0;

	i64 = 0;
	while (i64 <= 0xFFFFFFFF)
	{
		u.u = (unsigned int)i64;

		//Vector4 v1(u.f, u.f, u.f, u.f);
		//Vector4 v2(u.f, u.f, u.f, u.f);
		//glm::vec4 gv1(u.f, u.f, u.f, u.f);
		//glm::vec4 gv2(u.f, u.f, u.f, u.f);

		//Vector4 v3 = v1 + v2;
		//Vector4 v4 = UT_Vector4_Add(v1, v2);
		//glm::vec4 gv3 = gv1 + gv2;

		//Check(VectorEquals(v3, v4), u);

		//Check(VectorEquals(v3, gv3), u);

		++i64;

		if ((u.u >> 24) > counter)
		{
			printf("%x\n", counter);
			++counter;
		}
	}


	printf("done\n");
}

inline float RandF()
{
	// http://en.cppreference.com/w/cpp/numeric/random/RAND_MAX
	// RAND_MAX is at least 0x7FFF, so we use 15 bits
	IntFloatUnion u;
	u.i =
		((((unsigned __int32)rand() << 17) & 0xFFFE0000) |
			(((unsigned __int32)rand() << 2) & 0x0001FFFC) |
			(rand() & 0x00000002));
	return u.f;
}


inline float RandRangeF(float minR, float maxR)
{
	return minR + rand() / (float)RAND_MAX * (maxR - minR);
}

#define RandVectorIdx(idx) Vector4(\
	idx + 0 < FloatSpecialNum ? FloatSpecials[idx + 0].f : RandF(),\
	idx + 1 < FloatSpecialNum ? FloatSpecials[idx + 1].f : RandF(),\
	idx + 2 < FloatSpecialNum ? FloatSpecials[idx + 2].f : RandF(),\
	idx + 3 < FloatSpecialNum ? FloatSpecials[idx + 3].f : RandF())

#define RandRangeVectorIdx(idx, minR, maxR) Vector4(\
	idx + 0 < FloatSpecialNum ? FloatSpecials[idx + 0].f : RandRangeF(minR, maxR),\
	idx + 1 < FloatSpecialNum ? FloatSpecials[idx + 1].f : RandRangeF(minR, maxR),\
	idx + 2 < FloatSpecialNum ? FloatSpecials[idx + 2].f : RandRangeF(minR, maxR),\
	idx + 3 < FloatSpecialNum ? FloatSpecials[idx + 3].f : RandRangeF(minR, maxR))

#define MatrixLoopBegin(idx, count, name) for(int idx = 0; idx < (count); idx += 16)\
{ Matrix4 name (\
	RandVectorIdx(idx),\
	RandVectorIdx(idx + 4),\
	RandVectorIdx(idx + 8),\
	RandVectorIdx(idx + 12));

#define MatrixRangeLoopBegin(idx, count, name, minR, maxR) for(int idx = 0; idx < (count); idx += 16)\
{ Matrix4 name (\
	RandRangeVectorIdx(idx, minR, maxR),\
	RandRangeVectorIdx(idx + 4, minR, maxR),\
	RandRangeVectorIdx(idx + 8, minR, maxR),\
	RandRangeVectorIdx(idx + 12, minR, maxR));

#define VectorLoopBegin(idx, count, name) for(int idx = 0; idx < (count); idx += 4)\
{ Vector4 name = RandVectorIdx(idx);

#define VectorRangeLoopBegin(idx, count, name, minR, maxR) for(int idx = 0; idx < (count); idx += 4)\
{ Vector4 name = RandRangeVectorIdx(idx, minR, maxR);

#define FloatLoopBegin(idx, count, name) for(int idx = 0; idx < (count); ++idx)\
{ float name = idx < FloatSpecialNum ? FloatSpecials[idx].f : RandF();

#define FloatRangeLoopBegin(idx, count, name, minR, maxR) for(int idx = 0; idx < (count); ++idx)\
{ float name = idx < FloatSpecialNum ? FloatSpecials[idx].f : RandRangeF(minR, maxR);

#define LoopEnd }

typedef std::function<Matrix4(const Matrix4&, const Matrix4&)> FuncMM2M;
typedef std::function<Vector4(const Matrix4&, const Vector4&)> FuncMV2V;
typedef std::function<Matrix4(const Matrix4&)> FuncM2M;
typedef std::function<Vector4(const Vector4&, const Vector4&)> FuncVV2V;
typedef std::function<float(const Vector4&, const Vector4&)> FuncVV2F;
typedef std::function<Vector4(const Vector4&, float)> FuncVF2V;
typedef std::function<Vector4(const Vector4&)> FuncV2V;

template<typename T>
void RandomTest(T f1, T f2, T f3, float minR, float maxR, int count = 10000, int level = 0);

template<>
void RandomTest(FuncMM2M f1, FuncMM2M f2, FuncMM2M f3, float minR, float maxR, int count, int level)
{
	MatrixLoopBegin(i, count, v1)
	{
		MatrixLoopBegin(j, count, v2)
		{
			Check(v1, v2, f1(v1, v2), f2(v1, v2), f3(v1, v2), level);
		}
		LoopEnd
			printf("%d\n", i);
	}
	LoopEnd

	MatrixRangeLoopBegin(i, count, v1, minR, maxR)
	{
		MatrixRangeLoopBegin(j, count, v2, minR, maxR)
		{
			Check(v1, v2, f1(v1, v2), f2(v1, v2), f3(v1, v2), level);
		}
		LoopEnd
			printf("%d\n", i);
	}
	LoopEnd
}

template<>
void RandomTest(FuncMV2V f1, FuncMV2V f2, FuncMV2V f3, float minR, float maxR, int count, int level)
{
	MatrixLoopBegin(i, count, v1)
	{
		VectorLoopBegin(j, count, v2)
		{
			Check(v1, v2, f1(v1, v2), f2(v1, v2), f3(v1, v2), level);
		}
		LoopEnd
			printf("%d\n", i);
	}
	LoopEnd

	MatrixRangeLoopBegin(i, count, v1, minR, maxR)
	{
		VectorRangeLoopBegin(j, count, v2, minR, maxR)
		{
			Check(v1, v2, f1(v1, v2), f2(v1, v2), f3(v1, v2), level);
		}
		LoopEnd
			printf("%d\n", i);
	}
	LoopEnd
}

template<>
void RandomTest(FuncM2M f1, FuncM2M f2, FuncM2M f3, float minR, float maxR, int count, int level)
{
	MatrixLoopBegin(i, count, v)
	{
		Check(v, f1(v), f2(v), f3(v), level);

		printf("%d\n", i);
	}
	LoopEnd

	MatrixRangeLoopBegin(i, count, v, minR, maxR)
	{
		Check(v, f1(v), f2(v), f3(v), level);

		printf("%d\n", i);
	}
	LoopEnd
}

template<>
void RandomTest(FuncVV2V f1, FuncVV2V f2, FuncVV2V f3, float minR, float maxR, int count, int level)
{
	VectorLoopBegin(i, count, v1)
	{
		VectorLoopBegin(j, count, v2)
		{
			Check(v1, v2, f1(v1, v2), f2(v1, v2), f3(v1, v2), level);
		}
		LoopEnd
		printf("%d\n", i);
	}
	LoopEnd
		
	VectorRangeLoopBegin(i, count, v1, minR, maxR)
	{
		VectorRangeLoopBegin(j, count, v2, minR, maxR)
		{
			Check(v1, v2, f1(v1, v2), f2(v1, v2), f3(v1, v2), level);
		}
		LoopEnd
			printf("%d\n", i);
	}
	LoopEnd
}

template<>
void RandomTest(FuncVV2F f1, FuncVV2F f2, FuncVV2F f3, float minR, float maxR, int count, int level)
{
	VectorLoopBegin(i, count, v1)
	{
		VectorLoopBegin(j, count, v2)
		{
			Check(v1, v2, f1(v1, v2), f2(v1, v2), f3(v1, v2), level);
		}
		LoopEnd
			printf("%d\n", i);
	}
	LoopEnd

	VectorRangeLoopBegin(i, count, v1, minR, maxR)
	{
		VectorRangeLoopBegin(j, count, v2, minR, maxR)
		{
			Check(v1, v2, f1(v1, v2), f2(v1, v2), f3(v1, v2), level);
		}
		LoopEnd
			printf("%d\n", i);
	}
	LoopEnd
}

template<>
void RandomTest(FuncVF2V f1, FuncVF2V f2, FuncVF2V f3, float minR, float maxR, int count, int level)
{
	VectorLoopBegin(i, count, v)
	{
		FloatLoopBegin(j, count, f)
		{
			Check(v, f, f1(v, f), f2(v, f), f3(v, f), level);
		}
		LoopEnd
			printf("%d\n", i);
	}
	LoopEnd
		
	VectorRangeLoopBegin(i, count, v, minR, maxR)
	{
		FloatRangeLoopBegin(j, count, f, minR, maxR)
		{
			Check(v, f, f1(v, f), f2(v, f), f3(v, f), level);
		}
		LoopEnd
			printf("%d\n", i);
	}
	LoopEnd
}

template<>
void RandomTest(FuncV2V f1, FuncV2V f2, FuncV2V f3, float minR, float maxR, int count, int level)
{
	VectorLoopBegin(i, count, v)
	{
		//Vector4 t = VecRSqrt(v.mVec);
		//IntFloatUnion u;
		//PrintValue(t);

		Check(v, f1(v), f2(v), f3(v), level);

		printf("%d\n", i);
	}
	LoopEnd

	VectorRangeLoopBegin(i, count, v, minR, maxR)
	{
		Check(v, f1(v), f2(v), f3(v), level);

		printf("%d\n", i);
	}
	LoopEnd
}
