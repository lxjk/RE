#pragma once

//#include <stdio.h>
#include <stdlib.h>

#include "MathUtil.h"
#include "Vector4.h"
#include "Matrix4.h"
#include "Quat.h"

// By default Right Handed and Z Up
// X: right, Y: forward, Z: up

#define RE_Y_UP 1
#define RE_Z_UP 2

#define RE_UP_AXIS RE_Z_UP

#define RE_EULER_ZYX	1 // Z: Yaw, Y: Pitch, X: Roll
#define RE_EULER_ZXY	2 // Z: Yaw, X: Pitch, Y: Roll

#if RE_UP_AXIS == RE_Z_UP
#define RE_EULER_ANGLE	RE_EULER_ZXY
#else
#define RE_EULER_ANGLE	RE_EULER_ZYX
#endif

#define RE_DEPTH_ZERO_TO_ONE		1
#define RE_DEPTH_NEG_ONE_TO_ONE		2

#define RE_DEPTH_CLIP_SPACE		RE_DEPTH_NEG_ONE_TO_ONE


__forceinline Quat& AsQuat(Vector4& v) { return *(Quat*)(&v); }
__forceinline const Quat& AsQuat(const Vector4& v) { return *(Quat*)(&v); }

__forceinline Vector4& AsVector4(Quat& q) { return *(Vector4*)(&q); }
__forceinline const Vector4& AsVector4(const Quat& q) { return *(Vector4*)(&q); }

// euler is in degrees
inline Quat EulerToQuat(const Vector4_3& euler)
{
	Vec128 rad = VecMul(euler.m128, VecConst::Half_DegToRad);
#if ENABLE_VEC256
	Vec128 s, c;
	VecSinCos(rad, s, c);
	Vec256ZeroUpper();
#else
	Vec128 s = VecSin(rad);
	Vec128 c = VecCos(rad);
#endif

	Vec128 t0 = VecShuffle(s, c, 2,2,2,2); // sz, sz, cz, cz
	Vec128 t1 = VecShuffle(c, t0, 0,1,2,0); // cx, cy, cz, sz
	Vec128 t2 = VecShuffle(s, t0, 0,1,0,2); // sx, sy, sz, cz
	
	Vec128 t3 = VecSwizzle(s, 2,0,0,0); // sz, sx, sx, sx
	Vec128 B = VecMul(t1, t3); // cxsz, sxcy, sxcz, sxsz
	Vec128 t4 = VecSwizzle(c, 2,0,0,0); // cz, cx, cx, cx
	Vec128 A = VecMul(t2, t4); // sxcz, cxsy, cxsz, cxcz
	
	Vec128 t5 = VecSwizzle(s, 1,2,1,1); // sy, sz, sy, sy
	B = VecMul(B, t5); // cxsysz, sxcysz, sxsycz, sxsysz
	Vec128 t6 = VecSwizzle(c, 1,2,1,1); // cy, cz, cy, cy
	A = VecMul(A, t6); // sxcycz, cxsycz, cxcysz, cxcycz

#if RE_EULER_ANGLE == RE_EULER_ZXY
	Vec128 sign = VecConst::Sign_NPPN;
#else
	// default to RE_EULER_ZYX
	Vec128 sign = VecConst::Sign_NPNP;
#endif
	return VecAdd(A, VecMul(B, sign));
}

// euler is in degrees
inline Vector4_3 QuatToEuler(const Quat& q)
{
	Vec128 r;
#if RE_EULER_ANGLE == RE_EULER_ZXY
	const Vec128 const_1001 = VecSet(1.f, 0.f, 0.f, 1.f);
	
	Vec128 x = VecSet1(2.f * (q.y*q.z + q.x*q.w));
	Vec128 absX, signX;
	VecSignAndAbs(x, signX, absX);
	// 0 means small number (special case), 1 means normal case
	Vec128 mask = VecCmpLT(absX, VecConst::Vec_1_Minus_Small_Num);
	x = VecAsin(x);
	x = VecBlendVar(VecMul(VecConst::Vec_Half_PI, signX), x, mask);

	Vec128 t0 = VecSwizzle(q.m128, 1,1,2,2); // y, y, z, z
	Vec128 t1 = VecSwizzle(q.m128, 1,3,3,2); // y, w, w, z
	Vec128 t2 = VecMul(VecMul(t0, t1), VecConst::Sign_NPPN); // -yy, yw, zw, -zz
	Vec128 t3 = VecSwizzle1(q.m128, 0); // x, x, x, x
	Vec128 t4 = VecSwizzle(q.m128, 0,2,1,0); // x, z, y, x
	Vec128 t5 = VecSub(t2, VecMul(t3, t4)); // -yy-xx, yw-xz, zw-xy, -zz-xx
	Vec128 yz1 = VecAdd(t5, VecAdd(t5, const_1001));
	Vec128 yz0 = VecShuffle(q.m128, const_1001, 3,1,2,3); // w, y, 0, 1
	Vec128 atan_y = VecBlendVar(yz0, yz1, mask);
	Vec128 atan_x = VecSwizzle(atan_y, 0,0,3,3);
	Vec128 yz = VecAtan2(atan_y, atan_x);
	yz = VecMul(yz, VecBlendVar(VecConst::Vec_Two, VecConst::Vec_One, mask));

	r = VecBlend(yz, x, 1,0,0,1);
#else
	// default to RE_EULER_ZYX
	const Vec128 const_0101 = VecSet(0.f, 1.f, 0.f, 1.f);
	const Vec128 const_P2N2P2N2 = VecSet(2.f, -2.f, 2.f, -2.f);

	Vec128 y = VecSet1(2.f * (q.y*q.w - q.x*q.z));
	Vec128 absY, signY;
	VecSignAndAbs(y, signY, absY);
	// 0 means small number (special case), 1 means normal case
	Vec128 mask = VecCmpLT(absY, VecConst::Vec_1_Minus_Small_Num);
	y = VecAsin(y);
	y = VecBlendVar(VecMul(VecConst::Vec_Half_PI, signY), y, mask);

	Vec128 t0 = VecSwizzle_0022(q.m128); // x, x, z, z
	Vec128 t1 = VecSwizzle(q.m128, 3,0,3,2); // w, x, w, z
	Vec128 t2 = VecMul(t0, t1); // xw, xx, zw, zz
	Vec128 t3 = VecSwizzle1(q.m128, 1); // y, y, y, y
	Vec128 t4 = VecSwizzle(q.m128, 2,1,0,1); // z, y, x, y
	Vec128 t5 = VecAdd(t2, VecMul(t3, t4)); // xw+yz, xx+yy, zw+xy, zz+yy
	Vec128 xz1 = VecAdd(VecMul(t5, const_P2N2P2N2), const_0101);
	Vec128 xz0 = VecShuffle(q.m128, const_0101, 0,3,0,1); // x, w, 0, 1
	Vec128 atan_y = VecBlendVar(xz0, xz1, mask);
	Vec128 atan_x = VecSwizzle_1133(atan_y);
	Vec128 xz = VecAtan2(atan_y, atan_x);
	// we only care about x and z, so we can re-use const_P2N2P2N2
	xz = VecMul(xz, VecBlendVar(const_P2N2P2N2, VecConst::Vec_One, mask));

	r = VecBlend(xz, y, 0,1,0,1);
#endif

	return VecMul(r, VecConst::RadToDeg);
}

inline Quat Matrix4ToQuat(const Matrix4& m)
{
	Vec128 diag;
	diag = VecAdd(VecConst::Vec_One,	VecMul(VecSwizzle1(m.m128[0], 0), VecConst::Sign_PNNP));
	diag = VecAdd(diag,					VecMul(VecSwizzle1(m.m128[1], 1), VecConst::Sign_NPNP));
	diag = VecAdd(diag,					VecMul(VecSwizzle1(m.m128[2], 2), VecConst::Sign_NNPP));

	Vec128 t0 = VecShuffle(m.m128[1], m.m128[2], 0,2,0,1); // Y0, Y2, Z0, Z1
	Vec128 t1 = VecShuffle(m.m128[2], m.m128[0], 0,1,1,2); // Z0, Z1, X1, X2
	Vec128 t2 = VecShuffle(t0, t1, 1,2,2,1); // Y2, Z0, X1, Z1
	Vec128 t3 = VecShuffle(t1, t0, 1,3,0,3); // Z1, X2, Y0, Z1

	Vec128 t4 = VecAdd(t2, t3); // yz, xz, xy, ?
	Vec128 t5 = VecSub(t2, t3); // xw, yw, zw, 0
	
	Vec128 t6 = VecShuffle_0101(t4, t5); //yz, xz, xw, yw
	Vec128 t7 = VecShuffle(t4, t5, 1,0,3,2); // xz, yz, ?, zw
	Vec128 t8 = VecShuffle(t4, t6, 2,3,0,3); // xy, ?, yz, yw
	Vec128 t9 = VecShuffle(t4, t6, 3,2,1,2); // ?, xy, xz, xw

	// throughput: 3 blend = 1 shuffle
	Vec128 basew = VecBlend(t5, diag, 0,0,0,1); // xw, yw, zw, dw
	Vec128 basez = VecBlend(t7, diag, 0,0,1,0); // xz, yz, dz, zw
	Vec128 basey = VecBlend(t8, diag, 0,1,0,0); // xy, dy, yz, yw
	Vec128 basex = VecBlend(t9, diag, 1,0,0,0); // dx, xy, xz, xw

	Vec128 dx = VecSwizzle1(diag, 0);
	Vec128 dy = VecSwizzle1(diag, 1);
	Vec128 dz = VecSwizzle1(diag, 2);
	Vec128 dw = VecSwizzle1(diag, 3);

	// find out which one we want to use, max of (dx, dy, dz, dw)
	Vec128 cmp1 = VecCmpLT(dx, dy);
	Vec128 dv1 = VecBlendVar(dx, dy, cmp1);
	Vec128 base1 = VecBlendVar(basex, basey, cmp1);

	Vec128 cmp2 = VecCmpLT(dz, dw);
	Vec128 dv2 = VecBlendVar(dz, dw, cmp2);
	Vec128 base2 = VecBlendVar(basez, basew, cmp2);

	Vec128 cmp3 = VecCmpLT(dv1, dv2);
	Vec128 dv = VecBlendVar(dv1, dv2, cmp3);
	Vec128 base = VecBlendVar(base1, base2, cmp3);

	dv = VecDiv(VecConst::Vec_Half, VecSqrt(dv));

	return VecMul(base, dv);
}

#if 0 // this is faster but only works if w > small_num (angle != 180)
inline Quat Matrix4ToQuat(const Matrix4& m)
{
	Vec128 r;
	r = VecAdd(VecConst::Vec_One,	VecMul(VecSwizzle1(m.m128[0], 0), VecConst::Sign_PNNP));
	r = VecAdd(r,					VecMul(VecSwizzle1(m.m128[1], 1), VecConst::Sign_NPNP));
	r = VecAdd(r,					VecMul(VecSwizzle1(m.m128[2], 2), VecConst::Sign_NNPP));
	r = VecSqrt(r);

	Vec128 t0 = VecShuffle(m.m128[1], m.m128[2], 0,2,0,1); // Y0, Y2, Z0, Z1
	Vec128 t1 = VecShuffle(m.m128[2], m.m128[0], 0,1,1,2); // Z0, Z1, X1, X2
	Vec128 t2 = VecShuffle(t0, t1, 1,2,2,1); // Y2, Z0, X1, Z1
	Vec128 t3 = VecShuffle(t1, t0, 1,3,0,3); // Z1, X2, Y0, Z1

	Vec128 tw = VecSub(t2, t3); // xw, yw, zw, 0
	
	Vec128 s = VecBlendVar(VecConst::Vec_Neg_Half, VecConst::Vec_Half, VecCmpGE(tw, VecZero()));

	return VecMul(r, s);
}
#endif

inline Matrix4 QuatToMatrix4(const Quat& q)
{
	//Matrix4 r = {
	//	{1 - 2*q.y*q.y - 2*q.z*q.z,	2*q.x*q.y + 2*q.z*q.w,		2*q.x*q.z - 2*q.y*q.w,		0},
	//	{2*q.x*q.y - 2*q.z*q.w,		1 - 2*q.x*q.x - 2*q.z*q.z,	2*q.y*q.z + 2*q.x*q.w,		0},
	//	{2*q.x*q.z + 2*q.y*q.w,		2*q.y*q.z - 2*q.x*q.w,		1 - 2*q.x*q.x - 2*q.y*q.y,	0},
	//	{0,							0,							0,							1} };
	
	Vec128 xyz2 = VecAdd(q.m128, q.m128);		// 2x, 2y, 2z, 2w
	Vec128 zxy2 = VecSwizzle(xyz2, 2,0,1,3);	// 2z, 2x, 2y, 2w
	Vec128 yzx2 = VecSwizzle(xyz2, 1,2,0,3);	// 2y, 2z, 2x, 2w

	Vec128 t0 = VecAdd(	VecMul(zxy2, VecSwizzle(q.m128, 2,0,1,3)), 
						VecMul(yzx2, VecSwizzle(q.m128, 1,2,0,3))); // 2zz+2yy, 2xx+2zz, 2yy+2xx, 4ww
	t0 = VecSub(VecConst::Vec_One, t0); // 1-2zz-2yy, 1-2xx-2zz, 1-2yy-2xx, 1-4ww

	Vec128 w = VecSwizzle1(q.m128, 3);

	Vec128 t1 = VecAdd(VecMul(zxy2, w), VecMul(yzx2, q.m128)); // 2zw+2yx, 2xw+2zy, 2yw+2xz, 4ww
	Vec128 t2 = VecSub(VecMul(zxy2, q.m128), VecMul(yzx2, w)); // 2zx-2yw, 2xy-2zw, 2yz-2xw, 0

	Vec128 t3 = VecShuffle_0101(t0, t1); // 1-2zz-2yy, 1-2xx-2zz, 2zw+2yx, 2xw+2zy
	Vec128 t4 = VecShuffle(t0, t2, 1,2,1,3); // 1-2xx-2zz, 1-2yy-2xx, 2xy-2zw, 0
	Vec128 t5 = VecShuffle(t1, t2, 1,2,2,3); // 2xw+2zy, 2yw+2xz, 2yz-2xw, 0

	Matrix4 r;
	r.m128[0] = VecShuffle(t3, t2, 0,2,0,3);
	r.m128[1] = VecShuffle(t4, t5, 2,0,0,3);
	r.m128[2] = VecShuffle(t5, t4, 1,2,1,3);
	r.m128[3] = VecSet(0.f, 0.f, 0.f, 1.f);

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

	Vec128 yzx = VecSwizzle(q.m128, 1,2,0,3);	// y, z, x, w
	Vec128 yzx2 = VecAdd(yzx, yzx);				// 2y, 2z, 2x, 2w
	Vec128 yyzzxx2 = VecMul(yzx, yzx2);			// 2yy, 2zz, 2xx, 2ww
	Vec128 yxzyxz2 = VecMul(q.m128, yzx2);		// 2yx, 2zy, 2xz, 2ww
	Vec128 w = VecSwizzle1(q.m128, 3);
	Vec128 ywzwxw2 = VecMul(w, yzx2);			// 2yw, 2zw, 2xw, 2ww

	Vec128 t0 = VecAdd(VecSwizzle(yyzzxx2, 1,2,0,3), yyzzxx2);  // 2zz+2yy, 2xx+2zz, 2yy+2xx, 4ww
	t0 = VecSub(VecConst::Vec_One, t0);	// 1-2zz-2yy, 1-2xx-2zz, 1-2yy-2xx, 1-4ww

	Vec128 t1 = VecAdd(VecSwizzle(ywzwxw2, 1,2,0,3), yxzyxz2); // 2zw+2yx, 2xw+2zy, 2yw+2xz, 4ww
	Vec128 t2 = VecSub(VecSwizzle(yxzyxz2, 2,0,1,3), ywzwxw2); // 2zx-2yw, 2xy-2zw, 2yz-2xw, 0

	Vec128 t3 = VecShuffle_0101(t0, t1); // 1-2zz-2yy, 1-2xx-2zz, 2zw+2yx, 2xw+2zy
	Vec128 t4 = VecShuffle(t0, t2, 1,2,1,3); // 1-2xx-2zz, 1-2yy-2xx, 2xy-2zw, 0
	Vec128 t5 = VecShuffle(t1, t2, 1,2,2,3); // 2xw+2zy, 2yw+2xz, 2yz-2xw, 0

	Matrix4 r;
	r.m128[0] = VecShuffle(t3, t2, 0,2,0,3);
	r.m128[1] = VecShuffle(t4, t5, 2,0,0,3);
	r.m128[2] = VecShuffle(t5, t4, 1,2,1,3);
	r.m128[3] = VecSet(0.f, 0.f, 0.f, 1.f);

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

	Vec128 xyz = rotation.m128;
	Vec128 xyz2 = VecAdd(xyz, xyz);				// 2x, 2y, 2z, 2w
	Vec128 zxy2 = VecSwizzle(xyz2, 2,0,1,3);	// 2z, 2x, 2y, 2w
	Vec128 yzx2 = VecSwizzle(xyz2, 1,2,0,3);	// 2y, 2z, 2x, 2w

	Vec128 t0 = VecAdd(	VecMul(zxy2, VecSwizzle(xyz, 2,0,1,3)),
						VecMul(yzx2, VecSwizzle(xyz, 1,2,0,3))); // 2zz+2yy, 2xx+2zz, 2yy+2xx, 4ww
	t0 = VecSub(VecConst::Vec_One, t0); // 1-2zz-2yy, 1-2xx-2zz, 1-2yy-2xx, 1-4ww
	t0 = VecMul(t0, scale.m128);

	Vec128 w = VecSwizzle1(xyz, 3);

	Vec128 t1 = VecAdd(VecMul(zxy2, w), VecMul(yzx2, xyz)); // 2zw+2yx, 2xw+2zy, 2yw+2xz, 4ww
	t1 = VecMul(t1, scale.m128);
	Vec128 t2 = VecSub(VecMul(zxy2, xyz), VecMul(yzx2, w)); // 2zx-2yw, 2xy-2zw, 2yz-2xw, 0
	t2 = VecMul(t2, scale.m128);

	Vec128 t3 = VecShuffle_0101(t0, t1); // 1-2zz-2yy, 1-2xx-2zz, 2zw+2yx, 2xw+2zy
	Vec128 t4 = VecShuffle(t0, t2, 1,2,1,3); // 1-2xx-2zz, 1-2yy-2xx, 2xy-2zw, 0
	Vec128 t5 = VecShuffle(t1, t2, 1,2,2,3); // 2xw+2zy, 2yw+2xz, 2yz-2xw, 0

	Matrix4 r;
	r.m128[0] = VecShuffle(t3, t2, 0,2,0,3);
	r.m128[1] = VecShuffle(t4, t5, 2,0,0,3);
	r.m128[2] = VecShuffle(t5, t4, 1,2,1,3);
	r.m128[3] = VecBlend(translation.m128, VecConst::Vec_One, 0,0,0,1);

	return r;
}


// =================================================================
// Engine specifics

__forceinline float		Abs(float a) { return fabsf(a); }
__forceinline float		Floor(float a) { return floorf(a); }
__forceinline float		Ceil(float a) { return ceilf(a); }
__forceinline float		Round(float a) { return roundf(a); }
__forceinline float		Trunc(float a) { return truncf(a); }
__forceinline Vector4	Abs(Vector4 a) { return VecAbs(a.m128); }
__forceinline Vector4	Floor(Vector4 a) { return VecFloor(a.m128); }
__forceinline Vector4	Ceil(Vector4 a) { return VecCeil(a.m128); }
__forceinline Vector4	Round(Vector4 a) { return VecRound(a.m128); }
__forceinline Vector4	Trunc(Vector4 a) { return VecTrunc(a.m128); }


__forceinline int		Min(int a, int b) { return (a < b) ? a : b; }
__forceinline int		Max(int a, int b) { return (a > b) ? a : b; }
__forceinline float		Min(float a, float b) {	return (a < b) ? a : b; }
__forceinline float		Max(float a, float b) { return (a > b) ? a : b; }
// per component min
__forceinline Vector4	Min(Vector4 a, Vector4 b) { return VecMin(a.m128, b.m128); }
// per component max
__forceinline Vector4	Max(Vector4 a, Vector4 b) { return VecMax(a.m128, b.m128); }

__forceinline float		Clamp(float inValue, float inMin, float inMax) { return Min(Max(inValue, inMin), inMax); }
// per component clamp
__forceinline Vector4	Clamp(Vector4 inValue, Vector4 inMin, Vector4 inMax) { return Min(Max(inValue, inMin), inMax); }

__forceinline double	Lerp(double a, double b, double alpha) { return a + (b - a) * alpha; }
__forceinline float		Lerp(float a, float b, float alpha) { return a + (b - a) * alpha; }
__forceinline Vector4	Lerp(const Vector4& a, const Vector4& b, float alpha) { return a + (b - a) * alpha; }
// quat lerp does not maintain length (will return non-unit quat)
__forceinline Quat		Lerp(const Quat& a, const Quat& b, float alpha) { return a + (b - a) * alpha; }

__forceinline float		DegToRad(float angle) {	return angle * (PI / 180.f); }
__forceinline float		RadToDeg(float angle) {	return angle * (180.f / PI); }
__forceinline Vector4	DegToRad(const Vector4& angle) { return VecMul(angle.m128, VecConst::DegToRad); }
__forceinline Vector4	RadToDeg(const Vector4& angle) { return VecMul(angle.m128, VecConst::RadToDeg); }

__forceinline float		Sin(float a) { return sinf(a); }
__forceinline float		Cos(float a) { return cosf(a); }
__forceinline float		Tan(float a) { return tanf(a); }
__forceinline float		Asin(float a) { return asinf(a); }
__forceinline float		Acos(float a) { return acosf(a); }
__forceinline float		Atan(float a) { return atanf(a); }
__forceinline float		Atan2(float y, float x) { return atan2f(y, x); }
__forceinline Vector4	Sin(Vector4 a) { return VecSin(a.m128); }
__forceinline Vector4	Cos(Vector4 a) { return VecCos(a.m128); }
__forceinline Vector4	Tan(Vector4 a) { return VecTan(a.m128); }
__forceinline Vector4	Asin(Vector4 a) { return VecAsin(a.m128); }
__forceinline Vector4	Acos(Vector4 a) { return VecAcos(a.m128); }
__forceinline Vector4	Atan(Vector4 a) { return VecAtan(a.m128); }
__forceinline Vector4	Atan2(Vector4 y, Vector4 x) { return VecAtan2(y.m128, x.m128); }

__forceinline float		Sqrt(float a) { return sqrtf(a); }
__forceinline float		Exp2(float a) { return exp2f(a); }
__forceinline float		Exp(float a) { return expf(a); }
__forceinline float		Pow(float base, float exp) { return powf(base, exp); }
__forceinline Vector4	Sqrt(Vector4 a) { return VecSqrt(a.m128); }
__forceinline Vector4	Exp2(Vector4 a) { return VecExp2(a.m128); }
__forceinline Vector4	Exp(Vector4 a) { return VecExp(a.m128); }
__forceinline Vector4	Pow(Vector4 base, Vector4 exp) { return VecPow(base.m128, exp.m128); }

__forceinline float		Log2(float a) { return log2f(a); }
__forceinline float		Log10(float a) { return log10f(a); }
__forceinline float		Log(float a) { return logf(a); }
__forceinline float		Log(float a, float base) { return log2f(a) / log2f(base); }
__forceinline Vector4	Log2(Vector4 a) { return VecLog2(a.m128); }
__forceinline Vector4	Log10(Vector4 a) { return VecLog10(a.m128); }
__forceinline Vector4	Log(Vector4 a) { return VecLog(a.m128); }
__forceinline Vector4	Log(Vector4 a, Vector4 base) { return VecLog(a.m128, base.m128); }

__forceinline Vector4 VectorSelect(bool condition, const Vector4& T, const Vector4& F)
{
	return VecBlendVar(F.m128, T.m128, VecCmpNEQ(VecSet1(condition), VecZero()));
}
__forceinline Vector4 VectorSelectLE(float a, float b, const Vector4& T, const Vector4& F)
{
	return VecBlendVar(F.m128, T.m128, VecCmpLE(VecSet1(a), VecSet1(b)));
}
__forceinline Vector4 VectorSelectLT(float a, float b, const Vector4& T, const Vector4& F)
{
	return VecBlendVar(F.m128, T.m128, VecCmpLT(VecSet1(a), VecSet1(b)));
}
__forceinline Vector4 VectorSelectGE(float a, float b, const Vector4& T, const Vector4& F)
{
	return VecBlendVar(F.m128, T.m128, VecCmpGE(VecSet1(a), VecSet1(b)));
}
__forceinline Vector4 VectorSelectGT(float a, float b, const Vector4& T, const Vector4& F)
{
	return VecBlendVar(F.m128, T.m128, VecCmpGT(VecSet1(a), VecSet1(b)));
}

// [0.f, 1.f]
__forceinline float RandF()
{
	return rand() / (float)RAND_MAX;
}

__forceinline float RandRange(float minR, float maxR)
{
	return minR + RandF() * (maxR - minR);
}


// =================================================================
// Coordinates and Matrices specifics

inline Vector4_3 GetUpVector(const Quat& q)
{
#if RE_UP_AXIS == RE_Z_UP
	return q.Rotate(Vector4(0.f, 0.f, 1.f));
#else
	return q.Rotate(Vector4(0.f, 1.f, 0.f));
#endif
}

inline Vector4_3 GetForwardVector(const Quat& q)
{
#if RE_UP_AXIS == RE_Z_UP
	return q.Rotate(Vector4(0.f, 1.f, 0.f));
#else
	return q.Rotate(Vector4(0.f, 0.f, -1.f));
#endif
}

inline Vector4_3 GetRightVector(const Quat& q)
{
#if RE_UP_AXIS == RE_Z_UP
	return q.Rotate(Vector4(1.f, 0.f, 0.f));
#else
	return q.Rotate(Vector4(1.f, 0.f, 0.f));
#endif
}

// convert from model matrix to invert view matrix
__forceinline Matrix4 ToInvViewMatrix(const Matrix4& m)
{
#if RE_UP_AXIS == RE_Z_UP
	Matrix4 r = m;
	r.m128[1] = m.m128[2];
	r.m128[2] = VecNegate(m.m128[1]);
	return r;
#else
	return m;
#endif
}

inline Matrix4 MakeMatrixFromForward(const Vector4_3& forward, const Vector4_3& up)
{
	Vector4_3 right = forward.Cross3(up);
	//if (right.SizeSqr3() <= KINDA_SMALL_NUMBER)
	//	right = Vector4_3(1.f, 0.f, 0.f);
	//else
	//	right = right.GetNormalized3();
	right = VectorSelectLE(right.SizeSqr3(), KINDA_SMALL_NUMBER,
		Vector4_3(1.f, 0.f, 0.f),
		right.GetNormalized3());
	Vector4_3 newUp = right.Cross3(forward);

	Matrix4 r = Matrix4::Identity();

#if RE_UP_AXIS == RE_Z_UP
	r.SetAxes(right, forward, newUp);
#else
	r.SetAxes(right, newUp, -forward);
#endif

	return r;
}

inline Matrix4 MakeMatrixFromForward(const Vector4_3& forward)
{
#if RE_UP_AXIS == RE_Z_UP
	return MakeMatrixFromForward(forward, Vector4_3(0.f, 0.f, 1.f));
#else
	return MakeMatrixFromForward(forward, Vector4_3(0.f, 1.f, 0.f));
#endif
}


inline Matrix4 MakeMatrixFromViewForward(const Vector4_3& forward, const Vector4_3& up)
{
	Vector4_3 right = forward.Cross3(up);

	right = VectorSelectLE(right.SizeSqr3(), KINDA_SMALL_NUMBER,
		Vector4_3(1.f, 0.f, 0.f),
		right.GetNormalized3());
	Vector4_3 newUp = right.Cross3(forward);

	Matrix4 r = Matrix4::Identity();

	r.SetAxes(right, newUp, -forward);

	return r;
}

inline Matrix4 MakeMatrixFromViewForward(const Vector4_3& forward)
{
	return MakeMatrixFromViewForward(forward, Vector4_3(0.f, 1.f, 0.f));
}

inline Matrix4 MakeMatrixPerspectiveProj(float fov, float width, float height, float zNear, float zFar, float jitterX = 0, float jitterY = 0)
{
	float w = Cos(0.5f * fov) / Sin(0.5f * fov);
	float h = w * width / height;

	Matrix4 Result = Matrix4::Zero();
	Result.m[0][0] = w;
	Result.m[1][1] = h;
	Result.m[2][3] = -1.f;

	// negative since this is going to  * z / -z
	Result.m[2][0] = -jitterX * 2.f / width;
	Result.m[2][1] = -jitterY * 2.f / height;

#if RE_DEPTH_CLIP_SPACE == RE_DEPTH_ZERO_TO_ONE
	Result[2][2] = zFar / (zNear - zFar);
	Result[3][2] = -(zFar * zNear) / (zFar - zNear);
#else
	Result.m[2][2] = -(zFar + zNear) / (zFar - zNear);
	Result.m[3][2] = -(2 * zFar * zNear) / (zFar - zNear);
#endif

	return Result;
}

inline Matrix4 MakeMatrixOrthoProj(float left, float right, float bottom, float top, float zNear, float zFar
	, float width = 1, float height = 1, float jitterX = 0, float jitterY = 0)
{
	Matrix4 Result = Matrix4::Identity();
	Result.m[0][0] = 2.f / (right - left);
	Result.m[1][1] = 2.f / (top - bottom);
	Result.m[3][0] = -(right + left) / (right - left) + 2 * jitterX / width;
	Result.m[3][1] = -(top + bottom) / (top - bottom) + 2 * jitterY / height;

#if RE_DEPTH_CLIP_SPACE == RE_DEPTH_ZERO_TO_ONE
	Result.m[2][2] = -1.f / (zFar - zNear);
	Result.m[3][2] = -zNear / (zFar - zNear);
#else
	Result.m[2][2] = -2.f / (zFar - zNear);
	Result.m[3][2] = -(zFar + zNear) / (zFar - zNear);
#endif

	return Result;
}

inline Matrix4 PerspectiveAddJitter(const Matrix4& inMat, float width, float height, float jitterX, float jitterY)
{
	Matrix4 Result = inMat;

	// negative since this is going to  * z / -z
	Result.m[2][0] = -jitterX * 2.f / width;
	Result.m[2][1] = -jitterY * 2.f / height;

	return Result;
}

inline Matrix4 OrthoAddJitter(const Matrix4& inMat, float width, float height, float jitterX, float jitterY)
{
	Matrix4 Result = inMat;

	Result.m[3][0] += jitterX * 2.f / width;
	Result.m[3][1] += jitterY * 2.f / height;

	return Result;
}

// output 6 planes
// w = projMat[0][0], h = projMat[1][1], n = near plane dist, f = far plane dist
inline void GetFrustumPlanes(const Matrix4& viewMat, float w, float h, float n, float f, Plane* outPlanes)
{
	Vec128 vecW = VecSet1(w);
	Vec128 vecH = VecSet1(h);
	Vec128 normW = VecSet1(1.f / (1.f + w * w));
	Vec128 normH = VecSet1(1.f / (1.f + h * h));

	Matrix4 transViewMat = viewMat.GetTransposed();
	Vec128 right = transViewMat.m128[0];
	Vec128 up = transViewMat.m128[1];
	Vec128 forward = transViewMat.m128[2]; // negated at this point
	
	// T is translation(origin), in this case, zero vector
	// far plane: (-forward, dot(T, forward) + f)
	outPlanes[5] = VecAdd(forward, VecSet(0.f, 0.f, 0.f, f));

	// negate to get actual forward
	forward = VecNegate(forward);

	// near plane: (forward, -dot(T, forward) - n)
	outPlanes[0] = VecAdd(forward, VecSet(0.f, 0.f, 0.f, -n));
	// left plane: (forward + w*right, -dot(T, forward + w*right)) * normW
	outPlanes[1] = VecMul(VecAdd(forward, VecMul(right, vecW)), normW);
	// right plane: (forward - w*right, -dot(T, forward - w*right)) * normW
	outPlanes[2] = VecMul(VecSub(forward, VecMul(right, vecW)), normW);
	// top plane: (forward + h*up, -dot(T, forward + h*up)) * normH
	outPlanes[3] = VecMul(VecAdd(forward, VecMul(up, vecH)), normH);
	// bottom plane: (forward - h*up, -dot(T, forward - h*up)) * normH
	outPlanes[4] = VecMul(VecSub(forward, VecMul(up, vecH)), normH);
}

// =================================================================
// Geometry

inline Plane MakePlane(const Vector4_3& normal, const Vector4_3& point)
{
	// (n, -dot(n,p))
	return VecBlend(normal.m128, VecNegate(VecDot3V(normal.m128, point.m128)), 0, 0, 0, 1);
}

// return signed distance, positive if on the normal side
inline float PointToPlaneDist(const Vector4_3& point, const Plane& plane)
{
	// point.Dot3(plane) + plane.w
	//return VecDot(plane.m128, VecBlend(point.m128, VecConst::Vec_One, 0, 0, 0, 1));
	return plane.Dot4(point.GetOneW());
}

// return intersection point of line (defined by point & dir (assumed normalized)) and plane
inline Vector4_3 LinePlaneIntersection(const Vector4_3& point, const Vector4_3& dir, const Plane& plane)
{
	return point - dir * (PointToPlaneDist(point, plane) / dir.Dot3(plane));
}

// intersection between 2 AABB (minA, maxA) and (minB, maxB)
inline bool IsAABBIntersectAABB(const Vector4_3& minA, const Vector4_3& maxA, const Vector4_3& minB, const Vector4_3& maxB)
{
	Vec128 t0 = VecCmpLE(minA.m128, maxB.m128);
	Vec128 t1 = VecCmpGE(maxA.m128, minB.m128);
	int r = VecMoveMask(VecAnd(t0, t1));
	return (r & 0x7) == 0x7; // ignore w component
	//return maxB.x >= minA.x && maxB.y >= minA.y && maxB.z >= minA.z &&
	//	minB.x <= maxA.x && minB.y <= maxA.y && minB.z <= maxA.z;
}

// intersection between AABB (min, max) and sphere (center, radius)
inline bool IsAABBIntersectSphere(const Vector4_3& min, const Vector4_3& max, const Vector4_3& center, float radius)
{
	Vector4_3 boxCenter = (min + max) * 0.5f;
	Vector4_3 boxExtent = (max - min) * 0.5f;
	Vector4_3 diff = Max(Abs(center - boxCenter) - boxExtent, Vector4_3::Zero());
	return diff.SizeSqr3() <= radius * radius;
}

// intersection between AABB (min, max) and frustum (planes, planeCount) plane normals point inside the volume
inline bool IsAABBIntersectFrustum(const Vector4_3& min, const Vector4_3& max, Plane* planes, int planeCount)
{
	// prepare min/max
	Vec128 pMin = VecBlend(min.m128, VecConst::Vec_One, 0, 0, 0, 1);
	Vec128 pMax = VecBlend(max.m128, VecConst::Vec_One, 0, 0, 0, 1);
	for (int i = 0; i < planeCount; ++i)
	{
		Vec128 plane = planes[i].m128;
		// choose positive vertex, if this point is outside the volume, AABB is outside
		Vec128 pVert = VecBlendVar(pMin, pMax, VecCmpGE(plane, VecZero()));
		if (VecDot(pVert, plane) < 0)
			return false;
	}
	return true;
}

// this version will transfrom plane into space of AABB
// intersection between AABB (min, max) and frustum (planes, planeCount) plane normals point inside the volume
inline bool IsAABBIntersectFrustum(const Vector4_3& min, const Vector4_3& max, Plane* planes, int planeCount,
	const Matrix4& invMatAABB)
{
	// prepare min/max
	Vec128 pMin = VecBlend(min.m128, VecConst::Vec_One, 0, 0, 0, 1);
	Vec128 pMax = VecBlend(max.m128, VecConst::Vec_One, 0, 0, 0, 1);
	for (int i = 0; i < planeCount; ++i)
	{
		Vec128 plane = invMatAABB.TransformPlane(planes[i]).m128;
		// choose positive vertex, if this point is outside the volume, AABB is outside
		Vec128 pVert = VecBlendVar(pMin, pMax, VecCmpGE(plane, VecZero()));
		if (VecDot(pVert, plane) < 0)
			return false;
	}
	return true;
}

// intersection between OBB (permutedAxis, obbCenter, obbExtent) and sphere (center, radius)
// permutedScaledAxis has 3 elements: (X.x, Y.x, Z.x, ?), (X.y, Y.y, Z.y, ?), (X.z, Y.z, Z.z, ?)
inline bool IsOBBIntersectSphere(const Vector4* permutedAxis, const Vector4_3& obbCenter, const Vector4_3 obbExtent,
	const Vector4_3& center, float radius)
{
	Vec128 diff = VecSub(center.m128, obbCenter.m128);
	Vec128 t0 = VecMul(permutedAxis[0].m128, VecSwizzle1(diff, 0));
	Vec128 t1 = VecAdd(t0, VecMul(permutedAxis[1].m128, VecSwizzle1(diff, 1)));
	Vec128 t2 = VecAdd(t1, VecMul(permutedAxis[2].m128, VecSwizzle1(diff, 2))); // (DiffDotSx, PDiffDotSy, DiffDotSz, ?)
	Vec128 distVec = VecMax(VecSub(VecAbs(t2), obbExtent.m128), VecZero());	
	return VecDot3(distVec, distVec) <= radius * radius;
}

// intersection between OBB (permutedAxisCenter, obbExtent) and frustum (planes, planeCount) plane normals point inside the volume
// permutedScaledAxisCenter has 3 elements: (X.x, Y.x, Z.x, C.x), (X.y, Y.y, Z.y, C.y), (X.z, SY.z, SZ.z, C.z)
inline bool IsOBBIntersectFrustum(const Vector4* permutedAxisCenter, const Vector4_3 obbExtent, Plane* planes, int planeCount)
{
	for (int i = 0; i < planeCount; ++i)
	{
		Vec128 plane = planes[i].m128;
		Vec128 t0 = VecMul(permutedAxisCenter[0].m128, VecSwizzle1(plane, 0));
		Vec128 t1 = VecAdd(t0, VecMul(permutedAxisCenter[1].m128, VecSwizzle1(plane, 1)));
		Vec128 t2 = VecAdd(t1, VecMul(permutedAxisCenter[2].m128, VecSwizzle1(plane, 2))); // (PDotSx, PDotSy, PDotSz, PDotC)
		Vec128 t3 = VecBlend(VecAbs(VecMul(t2, obbExtent.m128)), VecAdd(t2, plane), 0, 0, 0, 1); // (Abs(PDotSx), Abs(PDotSy), Abs(PDotSz), signedDist)
		if (VecSum(t3) < 0)
			return false;
	}
	return true;
}

// intersection between sphere (center, radius) and frustum (planes, planeCount) plane normals point inside the volume
inline bool IsSphereIntersectFrustum(const Vector4_3& center, float radius, Plane* planes, int planeCount)
{
	for (int i = 0; i < planeCount; ++i)
	{
		if (PointToPlaneDist(center, planes[i]) < -radius)
			return false;
	}
	return true;
}

// outPackedVerts must contain at least 6 elements
// aspectRatio = height / width
inline void MakeFrustumPackedVerts(const Matrix4& invViewMat, float near, float far, float tanHalfAngle, float aspectRatio, Vector4* outPackedVerts)
{
	float x0 = near * tanHalfAngle;
	float y0 = x0 * aspectRatio;
	float x1 = far * tanHalfAngle;
	float y1 = x1 * aspectRatio;
	Matrix4 tmp;
	tmp.mLine[0] = invViewMat.TransformPoint(Vector4_3(x0, y0, -near));
	tmp.mLine[1] = invViewMat.TransformPoint(Vector4_3(-x0, y0, -near));
	tmp.mLine[2] = invViewMat.TransformPoint(Vector4_3(x0, -y0, -near));
	tmp.mLine[3] = invViewMat.TransformPoint(Vector4_3(-x0, -y0, -near));
	tmp = tmp.GetTransposed43();
	outPackedVerts[0] = tmp.mLine[0];
	outPackedVerts[1] = tmp.mLine[1];
	outPackedVerts[2] = tmp.mLine[2];
	tmp.mLine[0] = invViewMat.TransformPoint(Vector4_3(x1, y1, -far));
	tmp.mLine[1] = invViewMat.TransformPoint(Vector4_3(-x1, y1, -far));
	tmp.mLine[2] = invViewMat.TransformPoint(Vector4_3(x1, -y1, -far));
	tmp.mLine[3] = invViewMat.TransformPoint(Vector4_3(-x1, -y1, -far));
	tmp = tmp.GetTransposed43();
	outPackedVerts[3] = tmp.mLine[0];
	outPackedVerts[4] = tmp.mLine[1];
	outPackedVerts[5] = tmp.mLine[2];
}

// intersection between frustum (packedVerts) and frustum (planes, planeCount) plane normals point inside the volume
// first frustum is defined by 8 verts, packed in an array of 6 Vector4 as
// (x0, x1, x2, x3) (y0, y1, y2, y3) (z0, z1, z2, z3) (x4, x5, x6, x7) (y4, y5, y6, y7) (z4, z5, z6, z7)
inline bool IsFrustumIntersectFrustum(Vector4* packedVerts, Plane* planes, int planeCount)
{
	for (int i = 0; i < planeCount; ++i)
	{
		Vec128 plane = planes[i].m128;
		Vec128 planeX = VecSwizzle1(plane, 0);
		Vec128 planeY = VecSwizzle1(plane, 1);
		Vec128 planeZ = VecSwizzle1(plane, 2);
		Vec128 planeW = VecSwizzle1(plane, 3);
		Vec128 t0 = VecAdd(VecMul(packedVerts[0].m128, planeX), planeW);
		Vec128 t1 = VecAdd(VecMul(packedVerts[1].m128, planeY), t0);
		Vec128 t2 = VecAdd(VecMul(packedVerts[2].m128, planeZ), t1);
		Vec128 result = VecCmpLT(t2, VecZero());
		Vec128 t3 = VecAdd(VecMul(packedVerts[3].m128, planeX), planeW);
		Vec128 t4 = VecAdd(VecMul(packedVerts[4].m128, planeY), t3);
		Vec128 t5 = VecAdd(VecMul(packedVerts[5].m128, planeZ), t4);
		result = VecAnd(result, VecCmpLT(t5, VecZero()));
		// if all verts are outside the frustum, we are outside the frustum
		if (VecMoveMask(result) == 0xF)
			return false;
	}
	return true;
}