#include "stdio.h"
#include "stdlib.h"
#include "time.h"
#include <intrin.h>  

#include "../../3rdparty/glm/glm/glm.hpp"

#include "UnitTest.h"
#include "UT_Vector4.h"

#include "Windows.h"

__declspec(noinline)
unsigned __int64 rdtsc()
{
	return __rdtsc();
}

__declspec(noinline)
float Test_VV2F_SSE(Vector4 v1, Vector4 v2)
{
	return v1.Dot(v2);
}

__declspec(noinline)
float Test_VV2F_Flt(Vector4 v1, Vector4 v2)
{
	return UT_Vector4_Dot(v1, v2);
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

void Bench()
{
	LARGE_INTEGER performFreq;
	QueryPerformanceFrequency(&performFreq);
	double InvPerformanceFreq = (double)1.0 / (double)(performFreq.QuadPart);

	const int count = 10000000;
	//float tmp = 0;
	Vector4 tmp;
	unsigned __int64 acc_t0 = 0, acc_t1 = 0, acc_t2 = 0, acc_tt = 0;
	LARGE_INTEGER pc0, pc1, pc2, pc3;

	//Vector4 v1(RandF(), RandF(), RandF(), RandF());
	//Vector4 v2(RandF(), RandF(), RandF(), RandF());
	//float f = RandF();

	Vector4* v1 = (Vector4*)_aligned_malloc(sizeof(Vector4) * count, 16);
	Vector4* v2 = (Vector4*)_aligned_malloc(sizeof(Vector4) * count, 16);
	float* f = (float*)_aligned_malloc(sizeof(float) * count, 16);

	for (int i = 0; i < count; ++i)
	{
		v1[i] = Vector4(RandF(), RandF(), RandF(), RandF());
		v2[i] = Vector4(RandF(), RandF(), RandF(), RandF());
		f[i] = RandF();
	}

	// -- warmup
	for (int i = 0; i < count; ++i)
	{
		tmp += v1[i].x + v2[i].x + f[i];
	}
	// -- warmup

	// begin
	QueryPerformanceCounter(&pc0);

	{
		unsigned __int64 t0 = __rdtsc();
		for (int i = 0; i < count; ++i)
		{
			Vector4 r = v1[i] + v2[i];
			//float r = v1[i].Dot(v2[i]);
			tmp += r;
		}
		unsigned __int64 t1 = __rdtsc();
		acc_t0 += (t1 - t0);
	}

	QueryPerformanceCounter(&pc1);

	{
		unsigned __int64 t0 = __rdtsc();
		for (int i = 0; i < count; ++i)
		{
			Vector4 r = UT_Vector4_Add(v1[i], v2[i]);
			//float r = UT_Vector4_Dot3(v1[i], v2[i]);
			tmp += r;
		}
		unsigned __int64 t1 = __rdtsc();
		acc_t1 += (t1 - t0);
	}

	QueryPerformanceCounter(&pc2);

	{
		unsigned __int64 t0 = __rdtsc();
		for (int i = 0; i < count; ++i)
		{
			Vector4 r = UT_Vector3_Add(v1[i], v2[i]);
			//Vector4 r = UT_Vector4_GetNormalized3(v1[i]);
			//Vector4 r = GlmVec4ToVector4(glm::vec4(glm::normalize(glm::vec3(Vector4ToGlmVec4(v1[i]))), 0));
			//float r = UT_Vector4_Dot2_Glm(v1[i], v2[i]);
			tmp += r;
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

	//RandomTest<FuncVV2V>(
	//	[](const Vector4& v1, const Vector4& v2) { Vector4 r = v1.Cross3(v2); r.w = 0; return r;},
	//	[](const Vector4& v1, const Vector4& v2) { return UT_Vector4_Cross3(v1, v2);},
	//	[](const Vector4& v1, const Vector4& v2) { return UT_Vector4_Cross3_Glm(v1, v2);},
	//	-2, 2, 20000, 0
	//	);
	
	//RandomTest<FuncVV2F>(
	//	[](const Vector4& v1, const Vector4& v2) { return v1.Dot3(v2);},
	//	[](const Vector4& v1, const Vector4& v2) { return UT_Vector4_Dot3(v1, v2);},
	//	[](const Vector4& v1, const Vector4& v2) { return UT_Vector4_Dot3_Glm(v1, v2);},
	//	-2, 2, 20000, 0
	//	);

	//RandomTest<FuncVF2V>(
	//	[](const Vector4& v1, float f) { return v1 / f;},
	//	[](const Vector4& v1, float f) { return UT_Vector4_Div(v1, f);},
	//	[](const Vector4& v1, float f) { return UT_Vector4_Div_Glm(v1, f);},
	//	20000
	//	);
	
	//RandomTest<FuncV2V>(
	//	[](const Vector4& v1) { return v1.GetNormalized2();},
	//	[](const Vector4& v1) { return v1.GetNormalized2();},
	//	[](const Vector4& v1) { return (v1.SizeSqr2() < SMALL_NUMBER) ? Vector4::Zero() : UT_Vector4_GetNormalized2_Glm(v1);},
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