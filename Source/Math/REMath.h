#pragma once

#include "Vector4.h"
#include "Matrix4.h"
#include "Quat.h"


__forceinline Quat& AsQuat(Vector4& v) { return *(Quat*)(&v); }
__forceinline const Quat& AsQuat(const Vector4& v) { return *(Quat*)(&v); }

__forceinline Vector4& AsVector4(Quat& q) { return *(Vector4*)(&q); }
__forceinline const Vector4& AsVector4(const Quat& q) { return *(Vector4*)(&q); }

inline Quat EulerToQuat(float pitch, float yaw, float roll);
inline Quat EulerToQuat(const Vector4_3& euler);

inline Quat Matrix4ToQuat(const Matrix4& m)
{
	Vec128 r = VecSet(
		+m.m[0][0] - m.m[1][1] - m.m[2][2],
		-m.m[0][0] + m.m[1][1] - m.m[2][2],
		-m.m[0][0] - m.m[1][1] + m.m[2][2],
		+m.m[0][0] + m.m[1][1] + m.m[2][2]
		);

	Vec128 sub = VecSet(
		m.m[1][2] - m.m[2][1],
		m.m[2][0] - m.m[0][2],
		m.m[0][1] - m.m[1][0],
		0
		);

	r = VecAdd(r, VecSet1(1.f));
	r = VecSqrt(r);

	Vec128 sign = VecCmpGE(sub, VecZero());
	Vec128 s = VecBlend(VecSet1(-0.5f), VecSet1(0.5f), sign);

	return VecMul(r, s);
}

inline Quat Matrix4ToQuat2(const Matrix4& m)
{
	Vec128 r;
	r = VecAdd(VecSet1(1.f),	VecMul(VecSwizzle1(m.mVec[0], 0), VecSet(1.f, -1.f, -1.f, 1.f)));
	r = VecAdd(r,				VecMul(VecSwizzle1(m.mVec[1], 1), VecSet(-1.f, 1.f, -1.f, 1.f)));
	r = VecAdd(r,				VecMul(VecSwizzle1(m.mVec[2], 2), VecSet(-1.f, -1.f, 1.f, 1.f)));
	r = VecSqrt(r);

	Vec128 t0 = VecShuffle(m.mVec[1], m.mVec[2], 0,2,0,1); // Y0, Y2, Z0, Z1
	Vec128 t1 = VecShuffle(m.mVec[2], m.mVec[0], 0,1,1,2); // Z0, Z1, X1, X2
	Vec128 t2 = VecShuffle(t0, t1, 1,2,2,1); // Y2, Z0, X1, Z1
	Vec128 t3 = VecShuffle(t1, t0, 1,3,0,3); // Z1, X2, Y0, Z1
	Vec128 sign = VecCmpGE(VecSub(t2, t3), VecZero());
	Vec128 s = VecBlend(VecSet1(-0.5f), VecSet1(0.5f), sign);

	return VecMul(r, s);
}

inline Quat Matrix4ToQuat3(const Matrix4& m)
{
	Quat r;
	r.w = 0.5f * sqrt(m.m[0][0] + m.m[1][1] + m.m[2][2] + 1);
	float rqw = 0.25f / r.w;
	r.x = rqw * (m.m[1][2] - m.m[2][1]);
	r.y = rqw * (m.m[2][0] - m.m[0][2]);
	r.z = rqw * (m.m[0][1] - m.m[1][0]);
	return r;
}

inline Vector4_3 QuatToEuler(const Quat& q);

inline Matrix4 QuatToMatrix4(const Quat& q)
{
	//Matrix4 r = {
	//	{1 - 2*q.y*q.y - 2*q.z*q.z,	2*q.x*q.y + 2*q.z*q.w,		2*q.x*q.z - 2*q.y*q.w,		0},
	//	{2*q.x*q.y - 2*q.z*q.w,		1 - 2*q.x*q.x - 2*q.z*q.z,	2*q.y*q.z + 2*q.x*q.w,		0},
	//	{2*q.x*q.z + 2*q.y*q.w,		2*q.y*q.z - 2*q.x*q.w,		1 - 2*q.x*q.x - 2*q.y*q.y,	0},
	//	{0,							0,							0,							1} };
	
	Vec128 xyz2 = VecAdd(q.mVec, q.mVec);		// 2x, 2y, 2z, 2w
	Vec128 zxy2 = VecSwizzle(xyz2, 2,0,1,3);	// 2z, 2x, 2y, 2w
	Vec128 yzx2 = VecSwizzle(xyz2, 1,2,0,3);	// 2y, 2z, 2x, 2w

	Vec128 t0 = VecAdd(	VecMul(zxy2, VecSwizzle(q.mVec, 2,0,1,3)), 
						VecMul(yzx2, VecSwizzle(q.mVec, 1,2,0,3))); // 2zz+2yy, 2xx+2zz, 2yy+2xx, 4ww
	t0 = VecSub(VecSet1(1.f), t0); // 1-2zz-2yy, 1-2xx-2zz, 1-2yy-2xx, 1-4ww

	Vec128 w = VecSwizzle1(q.mVec, 3);

	Vec128 t1 = VecAdd(VecMul(zxy2, w), VecMul(yzx2, q.mVec)); // 2zw+2yx, 2xw+2zy, 2yw+2xz, 4ww
	Vec128 t2 = VecSub(VecMul(zxy2, q.mVec), VecMul(yzx2, w)); // 2zx-2yw, 2xy-2zw, 2yz-2xw, 0

	Vec128 t3 = VecShuffle_0101(t0, t1); // 1-2zz-2yy, 1-2xx-2zz, 2zw+2yx, 2xw+2zy
	Vec128 t4 = VecShuffle(t0, t2, 1,2,1,3); // 1-2xx-2zz, 1-2yy-2xx, 2xy-2zw, 0
	Vec128 t5 = VecShuffle(t1, t2, 1,2,2,3); // 2xw+2zy, 2yw+2xz, 2yz-2xw, 0

	Matrix4 r;
	r.mVec[0] = VecShuffle(t3, t2, 0,2,0,3);
	r.mVec[1] = VecShuffle(t4, t5, 2,0,0,3);
	r.mVec[2] = VecShuffle(t5, t4, 1,2,1,3);
	r.mVec[3] = VecSet(0.f, 0.f, 0.f, 1.f);

	return r;
}

#if 0 // this version has less instructions but more data dependency. Above is faster
inline Matrix4 QuatToMatrix4(const Quat& q)
{
	//Matrix4 r = {
	//	{1 - 2*q.y*q.y - 2*q.z*q.z,	2*q.x*q.y + 2*q.z*q.w,		2*q.x*q.z - 2*q.y*q.w,		0},
	//	{2*q.x*q.y - 2*q.z*q.w,		1 - 2*q.x*q.x - 2*q.z*q.z,	2*q.y*q.z + 2*q.x*q.w,		0},
	//	{2*q.x*q.z + 2*q.y*q.w,		2*q.y*q.z - 2*q.x*q.w,		1 - 2*q.x*q.x - 2*q.y*q.y,	0},
	//	{0,							0,							0,							1} };

	Vec128 yzx = VecSwizzle(q.mVec, 1,2,0,3);	// y, z, x, w
	Vec128 yzx2 = VecAdd(yzx, yzx);				// 2y, 2z, 2x, 2w
	Vec128 yyzzxx2 = VecMul(yzx, yzx2);			// 2yy, 2zz, 2xx, 2ww
	Vec128 yxzyxz2 = VecMul(q.mVec, yzx2);		// 2yx, 2zy, 2xz, 2ww
	Vec128 w = VecSwizzle1(q.mVec, 3);
	Vec128 ywzwxw2 = VecMul(w, yzx2);			// 2yw, 2zw, 2xw, 2ww

	Vec128 t0 = VecAdd(VecSwizzle(yyzzxx2, 1,2,0,3), yyzzxx2);  // 2zz+2yy, 2xx+2zz, 2yy+2xx, 4ww
	t0 = VecSub(VecSet1(1.f), t0);	// 1-2zz-2yy, 1-2xx-2zz, 1-2yy-2xx, 1-4ww

	Vec128 t1 = VecAdd(VecSwizzle(ywzwxw2, 1,2,0,3), yxzyxz2); // 2zw+2yx, 2xw+2zy, 2yw+2xz, 4ww
	Vec128 t2 = VecSub(VecSwizzle(yxzyxz2, 2,0,1,3), ywzwxw2); // 2zx-2yw, 2xy-2zw, 2yz-2xw, 0

	Vec128 t3 = VecShuffle_0101(t0, t1); // 1-2zz-2yy, 1-2xx-2zz, 2zw+2yx, 2xw+2zy
	Vec128 t4 = VecShuffle(t0, t2, 1,2,1,3); // 1-2xx-2zz, 1-2yy-2xx, 2xy-2zw, 0
	Vec128 t5 = VecShuffle(t1, t2, 1,2,2,3); // 2xw+2zy, 2yw+2xz, 2yz-2xw, 0

	Matrix4 r;
	r.mVec[0] = VecShuffle(t3, t2, 0,2,0,3);
	r.mVec[1] = VecShuffle(t4, t5, 2,0,0,3);
	r.mVec[2] = VecShuffle(t5, t4, 1,2,1,3);
	r.mVec[3] = VecSet(0.f, 0.f, 0.f, 1.f);

	return r;
}
#endif

inline Matrix4 MakeMatrix(const Vector4_3& translation, const Quat& rotation, const Vector4_3& scale)
{
	//Matrix4 r = {
	//	{1 - 2*q.y*q.y - 2*q.z*q.z,	2*q.x*q.y + 2*q.z*q.w,		2*q.x*q.z - 2*q.y*q.w,		0} * S0,
	//	{2*q.x*q.y - 2*q.z*q.w,		1 - 2*q.x*q.x - 2*q.z*q.z,	2*q.y*q.z + 2*q.x*q.w,		0} * S1,
	//	{2*q.x*q.z + 2*q.y*q.w,		2*q.y*q.z - 2*q.x*q.w,		1 - 2*q.x*q.x - 2*q.y*q.y,	0} * S2,
	//	{T0,						T1,							T2,							1} };

	Vec128 xyz = rotation.mVec;
	Vec128 xyz2 = VecAdd(xyz, xyz);				// 2x, 2y, 2z, 2w
	Vec128 zxy2 = VecSwizzle(xyz2, 2,0,1,3);	// 2z, 2x, 2y, 2w
	Vec128 yzx2 = VecSwizzle(xyz2, 1,2,0,3);	// 2y, 2z, 2x, 2w

	Vec128 t0 = VecAdd(	VecMul(zxy2, VecSwizzle(xyz, 2,0,1,3)),
						VecMul(yzx2, VecSwizzle(xyz, 1,2,0,3))); // 2zz+2yy, 2xx+2zz, 2yy+2xx, 4ww
	t0 = VecSub(VecSet1(1.f), t0); // 1-2zz-2yy, 1-2xx-2zz, 1-2yy-2xx, 1-4ww
	t0 = VecMul(t0, scale.mVec);

	Vec128 w = VecSwizzle1(xyz, 3);

	Vec128 t1 = VecAdd(VecMul(zxy2, w), VecMul(yzx2, xyz)); // 2zw+2yx, 2xw+2zy, 2yw+2xz, 4ww
	t1 = VecMul(t1, scale.mVec);
	Vec128 t2 = VecSub(VecMul(zxy2, xyz), VecMul(yzx2, w)); // 2zx-2yw, 2xy-2zw, 2yz-2xw, 0
	t2 = VecMul(t2, scale.mVec);

	Vec128 t3 = VecShuffle_0101(t0, t1); // 1-2zz-2yy, 1-2xx-2zz, 2zw+2yx, 2xw+2zy
	Vec128 t4 = VecShuffle(t0, t2, 1,2,1,3); // 1-2xx-2zz, 1-2yy-2xx, 2xy-2zw, 0
	Vec128 t5 = VecShuffle(t1, t2, 1,2,2,3); // 2xw+2zy, 2yw+2xz, 2yz-2xw, 0

	Matrix4 r;
	r.mVec[0] = VecShuffle(t3, t2, 0,2,0,3);
	r.mVec[1] = VecShuffle(t4, t5, 2,0,0,3);
	r.mVec[2] = VecShuffle(t5, t4, 1,2,1,3);
	r.mVec[3] = VecBlendConst(translation.mVec, VecSet1(1.f), 0,0,0,1);

	return r;
}