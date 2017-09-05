#include "stdio.h"
#include "stdlib.h"
#include "time.h"
#include <intrin.h>  

#include "../../3rdparty/glm/glm/glm.hpp"

#include "UnitTest.h"

#include "Windows.h"

__declspec(noinline)
unsigned __int64 rdtsc()
{
	return __rdtsc();
}

__declspec(noinline)
float Test_VV2F_SSE(Vector4 v1, Vector4 v2)
{
	return v1.Dot4(v2);
}

__declspec(noinline)
float Test_VV2F_Flt(Vector4 v1, Vector4 v2)
{
	return UT_Vector4_Dot4(v1, v2);
}

//__declspec(noinline)
inline float Test_VV2F_Glm(Vector4 v1, Vector4 v2)
{
	return 0;
}

__declspec(noinline)
float PostBenchLoop(Vector4 r,
	unsigned __int64 &acc_t0, unsigned __int64 t0, unsigned __int64 t1
)
{
	acc_t0 += (t1 - t0);
	return r.x;
}

__declspec(noinline)
float PostBenchLoop(glm::vec4 r,
	unsigned __int64 &acc_t0, unsigned __int64 t0, unsigned __int64 t1
)
{
	acc_t0 += (t1 - t0);
	return r.x;
}

__declspec(noinline)
float PostBenchLoop(float r,
	unsigned __int64 &acc_t0, unsigned __int64 t0, unsigned __int64 t1
	)
{
	acc_t0 += (t1 - t0);
	return r;
}

struct BenchData
{
	Matrix4 m1;
	//Matrix4 m2;
	//Vector4 v1;
	//Vector4 v2;
	//Quat q1;
	//Quat q2;
	//float f;
};

void Bench()
{
	LARGE_INTEGER performFreq;
	QueryPerformanceFrequency(&performFreq);
	double InvPerformanceFreq = (double)1.0 / (double)(performFreq.QuadPart);

	const int count = 10000000;
	//float tmp = 0;
	//Vector4 tmp;
	//Matrix4 tmp;
	Quat tmp;
	unsigned __int64 acc_t0 = 0, acc_t1 = 0, acc_t2 = 0, acc_tt = 0;
	LARGE_INTEGER pc0, pc1, pc2, pc3;

	//Vector4 v1(RandF(), RandF(), RandF(), RandF());
	//Vector4 v2(RandF(), RandF(), RandF(), RandF());
	//float f = RandF();

	BenchData* d = (BenchData*)_aligned_malloc(sizeof(BenchData) * count, 16);

	const float minR = -20.f;
	const float maxR = 20.f;

	for (int i = 0; i < count; ++i)
	{
		d[i].m1 = { 
			{ RandRangeF(minR, maxR), RandRangeF(minR, maxR), RandRangeF(minR, maxR), RandRangeF(minR, maxR) },
			{ RandRangeF(minR, maxR), RandRangeF(minR, maxR), RandRangeF(minR, maxR), RandRangeF(minR, maxR) },
			{ RandRangeF(minR, maxR), RandRangeF(minR, maxR), RandRangeF(minR, maxR), RandRangeF(minR, maxR) },
			{ RandRangeF(minR, maxR), RandRangeF(minR, maxR), RandRangeF(minR, maxR), RandRangeF(minR, maxR) } };
		//d[i].v1 = Vector4(RandF(), RandF(), RandF(), RandF());
		//d[i].v2 = Vector4(RandF(), RandF(), RandF(), RandF());
		//d[i].q1 = RandQuat();
		//d[i].q2 = RandQuat();
		//d[i].f = RandF();
	}

	// -- warmup
	for (int i = 0; i < count; ++i)
	{
		AsVector4(tmp) += *(float*)&d[i];
	}
	// -- warmup

	// begin
	QueryPerformanceCounter(&pc0);

	{
		unsigned __int64 t0 = __rdtsc();
		for (int i = 0; i < count; ++i)
		{
			tmp += Matrix4ToQuat2(d[i].m1);
			//tmp += d[i].m1.GetInverse();
			//tmp += UT_Matrix4_GetInverse_Intel(d[i].m1);
			//tmp += d[i].m1.InverseTransformPoint(d[i].v1);
			//tmp += d[i].m1.GetDeterminant();
		}
		unsigned __int64 t1 = __rdtsc();
		acc_t0 += (t1 - t0);
	}

	QueryPerformanceCounter(&pc1);

	{
		unsigned __int64 t0 = __rdtsc();
		for (int i = 0; i < count; ++i)
		{
			tmp += Matrix4ToQuat(d[i].m1);
			//tmp += d[i].m1.GetTransformInverseNoScale();
			//tmp += UT_Matrix4_GetInverse_UE4(d[i].m1);
			//tmp += d[i].m1.InverseTransformPointNoScale(d[i].v1);
			//tmp += d[i].m1.GetDeterminant();
		}
		unsigned __int64 t1 = __rdtsc();
		acc_t1 += (t1 - t0);
	}

	QueryPerformanceCounter(&pc2);

	{
		unsigned __int64 t0 = __rdtsc();
		for (int i = 0; i < count; ++i)
		{
			tmp += Matrix4ToQuat3(d[i].m1);
			//tmp += d[i].m1.GetTransformInverse();
			//tmp += UT_Matrix4_GetInverse_DirectX(d[i].m1);
			//tmp += d[i].m1.GetTransformInverse().TransformPoint(d[i].v1);
			//tmp += GlmMat4ToMatrix4(glm::inverse(Matrix4ToGlmMat4(d[i].m1)));
			//tmp += UT_Matrix4_GetDeterminant(d[i].m1);
		}
		unsigned __int64 t1 = __rdtsc();
		acc_t2 += (t1 - t0);
	}

	QueryPerformanceCounter(&pc3);

	double pt0 = (double)(pc1.QuadPart - pc0.QuadPart) * InvPerformanceFreq;
	double pt1 = (double)(pc2.QuadPart - pc1.QuadPart) * InvPerformanceFreq;
	double pt2 = (double)(pc3.QuadPart - pc2.QuadPart) * InvPerformanceFreq;

	printf("t0 SSE \t= %lld \t %f\n", acc_t0, pt0);
	printf("t1 flt \t= %lld \t %f\n", acc_t1, pt1);
	printf("t2 glm \t= %lld \t %f\n", acc_t2, pt2);
	printf("tt tmp \t= %lld\n", acc_tt);
	printf("tmp = %f\n", tmp);
	//printf("tmp = %f %f %f %f\n", tmp.x, tmp.y, tmp.z, tmp.w);
}


int main(int argc, char **argv)
{
	srand(time(0));
	
	Bench();

	//ExhaustTest();

	//TransformRandomTest<FuncM2Q>(
	//	[](const Matrix4& m1) { return Matrix4ToQuat2(m1);},
	//	[](const Matrix4& m1) { return Matrix4ToQuat2(m1);},
	//	[](const Matrix4& m1) { return Matrix4ToQuat3(m1);},
	//	-10000.f, 10000.f, 1.f, 1.f, 20000, 4
	//	);

	//RandomTest<FuncQ2M>(
	//	[](const Quat& q1) { return QuatToMatrix4_2(q1); },
	//	[](const Quat& q1) { return QuatToMatrix4_3(q1); },
	//	[](const Quat& q1) { glm::mat4 r = glm::mat4_cast(QuatToGlmQuat(q1)); return GlmMat4ToMatrix4(r); },
	//	-2, 2, 20000, 3
	//	);

	//RandomTest<FuncQV2V>(
	//	[](const Quat& q1, const Vector4& v2) { return UT_Quat_InverseRotate_Glm(q1, v2); },
	//	[](const Quat& q1, const Vector4& v2) { Vector4 r = q1.InverseRotate(v2); r.w = 0; return r;},
	//	[](const Quat& q1, const Vector4& v2) { return UT_Quat_InverseRotate_Glm(q1, v2); },
	//	-2000, 2000, 20000, 4
	//	);

	//RandomTest<FuncQQ2Q>(
	//	[](const Quat& q1, const Quat& q2) { return q1 * q2; },
	//	[](const Quat& q1, const Quat& q2) { return q1.MulUE4(q2); },
	//	[](const Quat& q1, const Quat& q2) { return UT_Quat_Mul_Glm(q1, q2); },
	//	-2, 2, 20000, 3
	//	);

	//TransformRandomTest<FuncM2M>(
	//	[](const Matrix4& m1) { return UT_Matrix4_GetInverseTransposed3(m1);},
	//	[](const Matrix4& m1) { return m1.GetInverseTransposed3(); },
	//	[](const Matrix4& m1) { return m1.GetInverseTransposed3(); },
	//	-10000.f, 10000.f, 0.01f, 1000.f, 20000, 3
	//	);

	//TransformRandomTest<FuncMV2V>(
	//	[](const Matrix4& m1, const Vector4& v2) { return m1.InverseTransformVector(v2); },
	//	[](const Matrix4& m1, const Vector4& v2) { Vector4 r = m1.GetTransformInverse().TransformVector(v2); r.w = 0; return r;},
	//	[](const Matrix4& m1, const Vector4& v2) { return m1.InverseTransformVector(v2);},
	//	-10000.f, 10000.f, 0.01f, 1000.f, 20000, 4
	//	);

	//RandomTest<FuncM2F>(
	//	[](const Matrix4& m1) { return m1.GetDeterminant(); },
	//	[](const Matrix4& m1) { return m1.GetDeterminant(); },
	//	[](const Matrix4& m1) { return UT_Matrix4_GetDeterminant(m1); },
	//	-2, 2, 20000, 3
	//	);

	//RandomTest<FuncM2M>(
	//	[](const Matrix4& m1) { return UT_Matrix4_GetInverse_Intel(m1); },
	//	[](const Matrix4& m1) { return m1.GetInverse(); },
	//	[](const Matrix4& m1) { return m1.GetInverse(); },
	//	-2, 2, 20000, 4
	//	);

	//RandomTest<FuncM2M>(
	//	[](const Matrix4& m1) { return m1.GetInverse(); },
	//	[](const Matrix4& m1) { Matrix4 r = m1.GetTransposed(); return UT_Matrix4_GetInverse_Intel(r).GetTransposed();},
	//	[](const Matrix4& m1) { Matrix4 r = m1.GetTransposed(); return UT_Matrix4_GetInverse_UE4(r).GetTransposed();},
	//	-2, 2, 20000, 3
	//	);

	//RandomTest<FuncMM2M>(
	//	[](const Matrix4& m1, const Matrix4& m2) { return m1 * m2; },
	//	[](const Matrix4& m1, const Matrix4& m2) { return UT_Matrix4_Mul_Matrix4(m1, m2); },
	//	[](const Matrix4& m1, const Matrix4& m2) { return UT_Matrix4_Mul_Matrix4_Glm(m1, m2); },
	//	-2, 2, 20000, 0
	//	);

	//RandomTest<FuncMV2V>(
	//	[](const Matrix4& m1, const Vector4& v2) { return m1 * v2; },
	//	[](const Matrix4& m1, const Vector4& v2) { return UT_Matrix4_Mul_Vector4(m1, v2); },
	//	[](const Matrix4& m1, const Vector4& v2) { return UT_Matrix4_Mul_Vector4(m1, v2); },
	//	-2, 2, 20000, 0
	//	);

	//RandomTest<FuncVV2V>(
	//	[](const Vector4& v1, const Vector4& v2) { return VecDot2V(v1.mVec, v2.mVec);},
	//	[](const Vector4& v1, const Vector4& v2) { return VecSet1(UT_Vector4_Dot2(v1, v2));},
	//	[](const Vector4& v1, const Vector4& v2) { return VecSet1(UT_Vector4_Dot2_Glm(v1, v2));},
	//	-2, 2, 20000, 0
	//	);
	
	//RandomTest<FuncVV2F>(
	//	[](const Vector4& v1, const Vector4& v2) { return VecDot2(v1.mVec, v2.mVec);},
	//	[](const Vector4& v1, const Vector4& v2) { return UT_Vector4_Dot2(v1, v2);},
	//	[](const Vector4& v1, const Vector4& v2) { return UT_Vector4_Dot2_Glm(v1, v2);},
	//	-2, 2, 20000, 0
	//	);

	//RandomTest<FuncVF2V>(
	//	[](const Vector4& v1, float f) { return v1 / f;},
	//	[](const Vector4& v1, float f) { return UT_Vector4_Div(v1, f);},
	//	[](const Vector4& v1, float f) { return UT_Vector4_Div_Glm(v1, f);},
	//	20000
	//	);
	
	//RandomTest<FuncV2V>(
	//	[](const Vector4& v1) { return v1.GetNormalized();},
	//	[](const Vector4& v1) { return v1.GetNormalizedFast();},
	//	[](const Vector4& v1) { return (v1.SizeSqr() < SMALL_NUMBER) ? Vector4::Zero() : UT_Vector4_GetNormalized_Glm(v1);},
	//	-2, 2, 20000, 3
	//	);
	
	//RandomTest<FuncV2V>(
	//	[](const Vector4& v1) { return VecInvSqrt(VecAbs(v1.mVec)); },
	//	[](const Vector4& v1) { return VecDiv(VecSet1(1), VecSqrt(VecAbs(v1.mVec))); },
	//	[](const Vector4& v1) { return VecDiv(VecSet1(1), VecSqrt(VecAbs(v1.mVec))); },
	//	-2, 2, 20000, 3
	//	);

	//Vector4 v3(RandF(), RandF(), RandF(), RandF());
	//Vector4 v2(5, 6, 7, 8);

	//v3.Normalize();
	//Vector4 v1(0, 0, 0, 0);
	//Vector4 v2(VecZero());

	//float r = Test_VV2F_SSE(v1, v3);
	//v3 = Vector4::Zero();
	//r += Test_VV2F_SSE(v2, v3);

	//PrintValue(v3);

	//Vector4 r1 = Test_SSE(v1, v2);
	//Vector4 r2 = Test_SSE_M(v1, v2);

	//printf("done %f", v3);

	printf("done\n");

	getchar();
	return 0;
}