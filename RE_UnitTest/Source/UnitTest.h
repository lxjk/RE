#pragma once

#include <stdlib.h>
#include <stdarg.h>
#include <functional>
#include "Windows.h"

#include "Math/REMath.h"
#include "UT_Vector4.h"
#include "UT_Matrix4.h"
#include "UT_Quat.h"

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


template<typename T>
void PrintValue(T v);


inline void _cdecl DebugLog(const char* format, ...)
{
	static char logBuf[512];
	static int bufCount = _countof(logBuf);
	va_list args;
	va_start(args, format);
	_vfprintf_l(stdout, format, NULL, args);
	_vsprintf_s_l(logBuf, bufCount, format, NULL, args);
	va_end(args);
	OutputDebugString(logBuf);
}


template<>
inline void PrintValue(float v)
{
	IntFloatUnion u;
	u.f = v;
	DebugLog("\t[%08x, %+.07e]", u.i, u.f);
	DebugLog("\n");
}

template<>
inline void PrintValue(Vector4 v)
{
	for (int i = 0; i < 4; ++i)
	{
		IntFloatUnion u;
		u.f = v.m[i];
		DebugLog("\t[%08x, %+.07e]", u.i, u.f);
	}
	DebugLog("\n");
}

template<>
inline void PrintValue(Matrix4 v)
{
	for (int i = 0; i < 4; ++i)
	{
		PrintValue(v.mLine[i]);
	}
}

template<>
inline void PrintValue(Quat q)
{
	PrintValue(AsVector4(q));
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

	//if (f1 == 0 || f2 == 0)
	//	return 3;

	float diff = abs(f1 - f2);
	float maxValue = max(1, max(abs(f1), abs(f2)));
	if (diff <= maxValue * 0.0001)
		return 3;

	if (abs(u1.i - u2.i) < 0x3)
		return 3;

	//if (abs(f1 - f2) < SMALL_NUMBER)
	//	return 3;

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
		max(CheckEquals(m1.mLine[0], m2.mLine[0]),
		max(CheckEquals(m1.mLine[1], m2.mLine[1]),
		max(CheckEquals(m1.mLine[2], m2.mLine[2]),
			CheckEquals(m1.mLine[3], m2.mLine[3]))));
}

template<>
inline int CheckEquals(Quat q1, Quat q2)
{
	return min(CheckEquals(AsVector4(q1), AsVector4(q2)), 
		CheckEquals(AsVector4(q1), -AsVector4(q2)));
}

template<typename T_op1, typename T_op2, typename T_r>
void Check(T_op1 v1, T_op2 v2, T_r r1, T_r r2, T_r r3, int level)
{
	int e2 = CheckEquals(r1, r2);
	if (e2 >level)
	{
		DebugLog("1-2 result: %d\n", e2);
		PrintValue(v1);
		PrintValue(v2);
		PrintValue(r1);
		PrintValue(r2);
		//__debugbreak();
	}

	int e3 = CheckEquals(r1, r3);
	if (e3 > level)
	{
		DebugLog("1-3 result: %d\n", e3);
		PrintValue(v1);
		PrintValue(v2);
		PrintValue(r1);
		PrintValue(r3);
		//__debugbreak();
	}
}

template<typename T_op, typename T_r>
void Check(T_op v, T_r r1, T_r r2, T_r r3, int level)
{
	int e2 = CheckEquals(r1, r2);
	if (e2 >level)
	{
		DebugLog("1-2 result: %d\n", e2);
		PrintValue(v);
		PrintValue(r1);
		PrintValue(r2);
		//__debugbreak();
	}

	int e3 = CheckEquals(r1, r3);
	if (e3 > level)
	{
		DebugLog("1-3 result: %d\n", e3);
		PrintValue(v);
		PrintValue(r1);
		PrintValue(r3);
		//__debugbreak();
	}
}

void ExhaustTest()
{
	IntFloatUnion u;
	IntFloatUnion ue;

	unsigned __int64 i64;

	unsigned int counter = 0;

	double prevError = 0;
	double error = 0;
	double maxError = 0;
	double count = 0;

	IntFloatUnion min_norm;
	min_norm.u = 0x00800000;

	i64 = 0;
	while (i64 <= 0xFFFFFFFF)
	//for(float t = 0.f; t < 3.15f * 0.5f; t += 0.001f)
	{
		u.u = (unsigned int)i64;
		//u.f = t;

		if (!IsFloatSpecial(u.i))
		{
			//Vec128 s, c;
			//s = VecSin(VecSet1(u.f));
			//float r1 = VecToFloat(s);
			//float r2 = sinf(u.f);
			float r1 = VecToFloat(VecSin(VecSet1(u.f)));
			float r2 = sinf(u.f);
			//float r1 = VecToFloat(VecPow(VecSet1(PI), VecSet1(u.f)));
			//float r2 = powf(PI, u.f);
			float e = abs(r1 - r2);
			ue.f = e;
			if (!IsFloatSpecial(ue.i) && abs(u.f) < 3.15f)
			{
				//if (abs(r2) >= 1.f)
				//	e /= abs(r2);
				error += e;
				count += 1;
				if (e > maxError)
				{
					maxError = e;
					if (e > 0.01 && e > prevError)
						DebugLog("input: %+.07e, r1: %+.07e, r2: %+.07e, e: %+.07e, avg: %+.07e, count %+.07e\n", u.f / PI * 180, r1, r2, e, (error / count), count);
				}
				prevError = e;
			}
		}

		++i64;

		if ((u.u >> 24) > counter)
		{
			printf("%x\n", counter);
			++counter;
		}
	}

	error /= count;
	printf("avg error : %f, max error : %f\n", error, maxError);

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

inline Quat RandQuat()
{
	Quat q(RandRangeF(-1.f, 1.f), RandRangeF(-1.f, 1.f), RandRangeF(-1.f, 1.f), RandRangeF(-1.f, 1.f));
	return q.GetNormalized();
}

inline Matrix4 RandTransformMatrix(float minT, float maxT, float minS, float maxS)
{
	Vector4 q(RandRangeF(-1.f, 1.f), RandRangeF(-1.f, 1.f), RandRangeF(-1.f, 1.f), RandRangeF(-1.f, 1.f));
	q = q.GetNormalized4();
	Matrix4 r = {
		{1 - 2*q.y*q.y - 2*q.z*q.z,	2*q.x*q.y + 2*q.z*q.w,		2*q.x*q.z - 2*q.y*q.w,		0},
		{2*q.x*q.y - 2*q.z*q.w,		1 - 2*q.x*q.x - 2*q.z*q.z,	2*q.y*q.z + 2*q.x*q.w,		0},
		{2*q.x*q.z + 2*q.y*q.w,		2*q.y*q.z - 2*q.x*q.w,		1 - 2*q.x*q.x - 2*q.y*q.y,	0},
		{RandRangeF(minT, maxT),	RandRangeF(minT, maxT),		RandRangeF(minT, maxT),		1} };
	r.mLine[0] *= RandRangeF(minS, maxS);
	r.mLine[1] *= RandRangeF(minS, maxS);
	r.mLine[2] *= RandRangeF(minS, maxS);
	return r;
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

#define QuatLoopBegin(idx, count, name) for(int idx = 0; idx < (count); idx += 4)\
{ Quat name = RandQuat();

#define TransformMatrixLoopBegin(idx, count, name, minT, maxT, minS, maxS) for(int idx = 0; idx < (count); idx += 16)\
{ Matrix4 name = RandTransformMatrix(minT, maxT, minS, maxS);

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


typedef std::function<Vector4(const Quat&)> FuncQ2V;
typedef std::function<Matrix4(const Quat&)> FuncQ2M;
typedef std::function<Quat(const Quat&)> FuncQ2Q;
typedef std::function<Vector4(const Quat&, const Vector4&)> FuncQV2V;
typedef std::function<Quat(const Quat&, const Quat&)> FuncQQ2Q;
typedef std::function<Matrix4(const Matrix4&, const Matrix4&)> FuncMM2M;
typedef std::function<Vector4(const Matrix4&, const Vector4&)> FuncMV2V;
typedef std::function<Quat(const Matrix4&)> FuncM2Q;
typedef std::function<Matrix4(const Matrix4&)> FuncM2M;
typedef std::function<float(const Matrix4&)> FuncM2F;
typedef std::function<Vector4(const Vector4&, const Vector4&)> FuncVV2V;
typedef std::function<float(const Vector4&, const Vector4&)> FuncVV2F;
typedef std::function<Vector4(const Vector4&, float)> FuncVF2V;
typedef std::function<Vector4(const Vector4&)> FuncV2V;
typedef std::function<Quat(const Vector4&)> FuncV2Q;

template<typename T>
void RandomTest(T f1, T f2, T f3, float minR, float maxR, int count = 10000, int level = 0);

template<>
void RandomTest(FuncQ2V f1, FuncQ2V f2, FuncQ2V f3, float minR, float maxR, int count, int level)
{
	QuatLoopBegin(i, count, v1)
	{
		Check(v1, f1(v1), f2(v1), f3(v1), level);
		printf("%d\n", i);
	}
	LoopEnd
}

template<>
void RandomTest(FuncQ2M f1, FuncQ2M f2, FuncQ2M f3, float minR, float maxR, int count, int level)
{
	QuatLoopBegin(i, count, v1)
	{
		Check(v1, f1(v1), f2(v1), f3(v1), level);
		printf("%d\n", i);
	}
	LoopEnd
}

template<>
void RandomTest(FuncQ2Q f1, FuncQ2Q f2, FuncQ2Q f3, float minR, float maxR, int count, int level)
{
	QuatLoopBegin(i, count, v1)
	{
		Check(v1, f1(v1), f2(v1), f3(v1), level);
		printf("%d\n", i);
	}
	LoopEnd
}

template<>
void RandomTest(FuncQV2V f1, FuncQV2V f2, FuncQV2V f3, float minR, float maxR, int count, int level)
{
	//QuatLoopBegin(i, count, v1)
	//{
	//	VectorLoopBegin(j, count, v2)
	//	{
	//		Check(v1, v2, f1(v1, v2), f2(v1, v2), f3(v1, v2), level);
	//	}
	//	LoopEnd
	//	printf("%d\n", i);
	//}
	//LoopEnd

		
	QuatLoopBegin(i, count, v1)
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
void RandomTest(FuncQQ2Q f1, FuncQQ2Q f2, FuncQQ2Q f3, float minR, float maxR, int count, int level)
{
	QuatLoopBegin(i, count, v1)
	{
		QuatLoopBegin(j, count, v2)
		{
			Check(v1, v2, f1(v1, v2), f2(v1, v2), f3(v1, v2), level);
		}
		LoopEnd
			printf("%d\n", i);
	}
	LoopEnd
}

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
void RandomTest(FuncM2F f1, FuncM2F f2, FuncM2F f3, float minR, float maxR, int count, int level)
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
		//Vector4 t = VecRSqrt(v.m128);
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

template<>
void RandomTest(FuncV2Q f1, FuncV2Q f2, FuncV2Q f3, float minR, float maxR, int count, int level)
{
	VectorRangeLoopBegin(i, count, v, minR, maxR)
	{
		Check(v, f1(v), f2(v), f3(v), level);

		printf("%d\n", i);
	}
	LoopEnd
}

template<typename T>
void TransformRandomTest(T f1, T f2, T f3, float minT, float maxT, float minS, float maxS, int count = 10000, int level = 0);


template<>
void TransformRandomTest(FuncM2Q f1, FuncM2Q f2, FuncM2Q f3, float minT, float maxT, float minS, float maxS, int count, int level)
{
	TransformMatrixLoopBegin(i, count, v, minT, maxT, minS, maxS)
	{
		Check(v, f1(v), f2(v), f3(v), level);

		printf("%d\n", i);
	}
	LoopEnd
}

template<>
void TransformRandomTest(FuncM2M f1, FuncM2M f2, FuncM2M f3, float minT, float maxT, float minS, float maxS, int count, int level)
{
	TransformMatrixLoopBegin(i, count, v, minT, maxT, minS, maxS)
	{
		Check(v, f1(v), f2(v), f3(v), level);

		printf("%d\n", i);
	}
	LoopEnd
}

template<>
void TransformRandomTest(FuncMV2V f1, FuncMV2V f2, FuncMV2V f3, float minT, float maxT, float minS, float maxS, int count, int level)
{
	TransformMatrixLoopBegin(i, count, v1, minT, maxT, minS, maxS)
	{
		VectorRangeLoopBegin(j, count, v2, minT, maxT)
		{
			Check(v1, v2, f1(v1, v2), f2(v1, v2), f3(v1, v2), level);
		}
		LoopEnd

		printf("%d\n", i);
	}
	LoopEnd
}