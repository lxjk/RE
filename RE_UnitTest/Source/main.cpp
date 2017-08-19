#include "stdio.h"
#include "stdlib.h"
#include "time.h"
#include <intrin.h>  

#include "../../3rdparty/glm/glm/glm.hpp"

#include "UT_Vector4.h"

union IntFloatUnion
{
	unsigned __int32 i;
	float f;
};

__declspec(noinline)
unsigned __int64 rdtsc()
{
	return __rdtsc();
}

__declspec(noinline)
Vector4 Test_Float(Vector4 v1, Vector4 v2)
{
	return Vector4(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w);
}

__declspec(noinline)
Vector4 Test_SSE(Vector4 v1, Vector4 v2)
{
	return -v1;
}

__declspec(noinline)
Vector4 Test_SSE(Vector4 v1)
{
	return -v1;
}

__declspec(noinline)
Vector4 Test_SSE_M(Vector4 v1, Vector4 v2)
{
	Vec128 t0 = _mm_load_ps(v1.m);
	Vec128 t1 = _mm_load_ps(v2.m);
	Vector4 r;
	_mm_store_ps(r.m, _mm_add_ps(t0, t1));
	return r;
}

__declspec(noinline)
Vector4 Test_SSE_M(Vector4 v1)
{
	return _mm_xor_ps(v1.mVec, _mm_set1_ps(-0.f));
}

__declspec(noinline)
float PostBenchLoop(Vector4 r1, Vector4 r2, glm::vec4 r3, 
	unsigned __int64 &acc_t0, unsigned __int64 &acc_t1, unsigned __int64 &acc_t2,
	unsigned __int64 t0, unsigned __int64 t1, unsigned __int64 t2, unsigned __int64 t3
)
{
	acc_t0 += (t1 - t0);
	acc_t1 += (t2 - t1);
	return r1.x + r2.x + r3.x;
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

void Bench()
{
	int count = 1000000;
	float tmp = 0;
	unsigned __int64 acc_t0 = 0, acc_t1 = 0, acc_t2;

#if 0
	for (int i = 0; i < count; ++i)
	{
		Vector4 v1(rand(), rand(), rand(), rand());
		Vector4 v2(rand(), rand(), rand(), rand());
		glm::vec4 gv1(rand(), rand(), rand(), rand());
		glm::vec4 gv2(rand(), rand(), rand(), rand());

		unsigned __int64 t0 = rdtsc();
		Vector4 v3 = v1 + v2;
		unsigned __int64 t1 = rdtsc();
		Vector4 v4 = UT_Vector4_Add(v1, v2);
		unsigned __int64 t2 = rdtsc();
		glm::vec4 gv3 = gv1 + gv2;
		unsigned __int64 t3 = rdtsc();

		tmp += PostBenchLoop(v3, v4, gv3, acc_t0, acc_t1, acc_t2, t0, t1, t2, t3);
	}
#else

	for (int i = 0; i < count; ++i)
	{
		Vector4 v1(rand(), rand(), rand(), rand());
		Vector4 v2(rand(), rand(), rand(), rand());

		unsigned __int64 t0 = rdtsc();
		Vector4 v3 = v1 + v2;
		unsigned __int64 t1 = rdtsc();

		tmp += PostBenchLoop(v3, acc_t0, t0, t1);
	}

	for (int i = 0; i < count; ++i)
	{
		Vector4 v1(rand(), rand(), rand(), rand());
		Vector4 v2(rand(), rand(), rand(), rand());

		unsigned __int64 t0 = rdtsc();
		Vector4 v3 = UT_Vector4_Add(v1, v2);
		unsigned __int64 t1 = rdtsc();

		tmp += PostBenchLoop(v3, acc_t1, t0, t1);
	}

	for (int i = 0; i < count; ++i)
	{
		glm::vec4 v1(rand(), rand(), rand(), rand());
		glm::vec4 v2(rand(), rand(), rand(), rand());

		unsigned __int64 t0 = rdtsc();
		glm::vec4 v3 = v1 + v2;
		unsigned __int64 t1 = rdtsc();

		tmp += PostBenchLoop(v3, acc_t2, t0, t1);
	}
#endif

	printf("t0 SSE \t= %lld\n", acc_t0);
	printf("t1 flt \t= %lld\n", acc_t1);
	printf("t2 glm \t= %lld\n", acc_t2);
	printf("tmp = %f\n", tmp);
}

#define FloatSpecial(i) ((i & 0x7F800000) == 0x7F800000)
#define IsQNaN(i) (FloatSpecial(i) && (i & 0x00400000) == 0x00400000)
#define IsSNaN(i) (FloatSpecial(i) && (i & 0x00400000) == 0)
#define max(a, b) (a > b ? a : b)

// 0 same, 1 within threshould, 2 different special value, 3 differnt valid float
inline int FloatEquals(float f1, float f2)
{
	IntFloatUnion u1, u2;
	u1.f = f1;
	u2.f = f2;
	
	if (u1.i == u2.i)
		return 0;

	if (FloatSpecial(u1.i) || FloatSpecial(u2.i))
	{
		// if same type of NaN, let it pass
		if ((IsQNaN(u1.i) && IsQNaN(u2.i)) ||
			(IsSNaN(u1.i) && IsSNaN(u2.i)))
			return 0;
		else
			return 2;
	}

	if (abs(f1 - f2) < SMALL_NUMBER)
		return 1;

	if (FloatSpecial(u1.i) || FloatSpecial(u2.i))
		return 2;

	return 3;
}

#define VectorEquals(v1, v2) max(FloatEquals(v1.x, v2.x),max(FloatEquals(v1.y, v2.y),max(FloatEquals(v1.z, v2.z),FloatEquals(v1.w, v2.w))))

//inline int VectorEquals(const Vector4& v1, const Vector4& v2)
//{
//	return max(FloatEquals(v1.x, v2.x),
//		max(FloatEquals(v1.y, v2.y),
//		max(FloatEquals(v1.z, v2.z),
//		FloatEquals(v1.w, v2.w))));
//}
//
//inline int VectorEquals(const Vector4& v1, const glm::vec4& v2)
//{
//	return max(FloatEquals(v1.x, v2.x),
//		max(FloatEquals(v1.y, v2.y),
//		max(FloatEquals(v1.z, v2.z),
//		FloatEquals(v1.w, v2.w))));
//}

#define PrintVector4(v) {\
	for(int i = 0; i < 4; ++i) {\
		IntFloatUnion u;\
		u.f = v.m[i];\
		printf("\t[%08x, %+.04e]", u.i, u.f);\
	}\
	printf("\n");}

#define PrintGlmVec4(v) {\
	for(int i = 0; i < 4; ++i) {\
		IntFloatUnion u;\
		u.f = v[i]; printf("\t[%08x, %+.04e]", u.i, u.f);\
	}\
	printf("\n");}

#define PrintFloat(f) {\
	IntFloatUnion u;\
	u.f = f;\
	printf("\t[%08x, %+.04e]", u.i, u.f);\
	printf("\n");}

inline void Check(int result, IntFloatUnion u)
{
	if (result > 0)
	{
		printf("result: %d, i: %x, f: %f\n", result, u.i, u.f);
		__debugbreak();
	}
}

inline void Check(int result, Vector4 op, Vector4 r1, Vector4 r2)
{
	if (result > 0)
	{
		printf("result: %d\n", result);
		for (int i = 0; i < 4; ++i)
		{
			IntFloatUnion u_op, u_r1, u_r2;
			u_op.f = op.m[i];
			u_r1.f = r1.m[i];
			u_r2.f = r2.m[i];
			printf("\t%d: i: %x, f: %f --> i: %x, f: %f | i: %x, f: %f\n", 
				i, u_op.i, u_op.f, 
				u_r1.i, u_r1.f, u_r2.i, u_r2.f);
		}
		__debugbreak();
	}
}

inline void Check(int result, Vector4 op1, Vector4 op2, Vector4 r1, Vector4 r2)
{
	if (result > 0)
	{
		printf("result: %d\n", result);
		for (int i = 0; i < 4; ++i)
		{
			IntFloatUnion u_op1, u_op2, u_r1, u_r2;
			u_op1.f = op1.m[i];
			u_op2.f = op2.m[i];
			u_r1.f = r1.m[i];
			u_r2.f = r2.m[i];
			printf("\t%d: i: %x, f: %f +++ i: %x, f: %f\n\t\t --> i: %x, f: %f | i: %x, f: %f\n", 
				i, u_op1.i, u_op1.f, u_op2.i, u_op2.f,
				u_r1.i, u_r1.f, u_r2.i, u_r2.f);
		}
		__debugbreak();
	}
}

inline void Check(int result, Vector4 op1, Vector4 op2, Vector4 r1, glm::vec4 r2)
{
	if (result > 0)
	{		
		printf("result: %d\n", result);
		for (int i = 0; i < 4; ++i)
		{
			IntFloatUnion u_op1, u_op2, u_r1, u_r2;
			u_op1.f = op1.m[i];
			u_op2.f = op2.m[i];
			u_r1.f = r1.m[i];
			u_r2.f = r2[i];
			printf("\t%d: i: %x, f: %f +++ i: %x, f: %f\n\t\t --> i: %x, f: %f | i: %x, f: %f\n",
				i, u_op1.i, u_op1.f, u_op2.i, u_op2.f,
				u_r1.i, u_r1.f, u_r2.i, u_r2.f);

		}
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
		u.i = i64;

		Vector4 v1(u.f, u.f, u.f, u.f);
		Vector4 v2(u.f, u.f, u.f, u.f);
		glm::vec4 gv1(u.f, u.f, u.f, u.f);
		glm::vec4 gv2(u.f, u.f, u.f, u.f);

		Vector4 v3 = v1 + v2;
		Vector4 v4 = UT_Vector4_Add(v1, v2);
		glm::vec4 gv3 = gv1 + gv2;

		Check(VectorEquals(v3, v4), u);

		Check(VectorEquals(v3, gv3), u);

		++i64;

		if ((u.i >> 24) > counter)
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

void RandomTest()
{
	// float bit
	// 31 30-23(8) 22-0(23)
	//sign exp     frac

	IntFloatUnion specials[] =
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
	int spNum = _countof(specials);

	int count = 40000;
	for (int i = 0; i < count; i += 4)
	{
		Vector4 v1(
			i+0 < spNum ? specials[i+0].f : RandF(),
			i+1 < spNum ? specials[i+1].f : RandF(),
			i+2 < spNum ? specials[i+2].f : RandF(),
			i+3 < spNum ? specials[i+3].f : RandF());
		glm::vec4 gv1(v1.x, v1.y, v1.z, v1.w);

		for (int j = 0; j < count; j+=4)
		{
			Vector4 v2(
				j+0 < spNum ? specials[j+0].f : RandF(),
				j+1 < spNum ? specials[j+1].f : RandF(),
				j+2 < spNum ? specials[j+2].f : RandF(),
				j+3 < spNum ? specials[j+3].f : RandF());
			glm::vec4 gv2(v2.x, v2.y, v2.z, v2.w);

			Vector4 v3 = v1 + v2;
			Vector4 v4 = UT_Vector4_Add(v1, v2);
			glm::vec4 gv3 = gv1 + gv2;
			
			Check(VectorEquals(v3, v4), v1, v2, v3, v4);

			Check(VectorEquals(v3, gv3), v1, v2, v3, gv3);
		}

		printf("%d\n", i);
	}
}

int main(int argc, char **argv)
{
	srand(time(0));

	Vector4 v1(RandF(), RandF(), RandF(), RandF());

	PrintVector4(v1);

	//Bench();

	//ExhaustTest();
	//RandomTest();

	//Vector4 v(1, 2, 3, 4);

	//v = Test_SSE(v);
	//v = Test_SSE_M(v);

	//printf("%f", v.x);

	getchar();
	return 0;
}