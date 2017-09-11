#pragma once

#include "Vector4.h"
#include "Matrix4.h"
#include "Quat.h"


__forceinline Quat& AsQuat(Vector4& v) { return *(Quat*)(&v); }
__forceinline const Quat& AsQuat(const Vector4& v) { return *(Quat*)(&v); }

__forceinline Vector4& AsVector4(Quat& q) { return *(Vector4*)(&q); }
__forceinline const Vector4& AsVector4(const Quat& q) { return *(Vector4*)(&q); }

inline Quat EulerToQuat(const Vector4_3& euler)
{
	Vec128 s = VecSin(euler.mVec);
	Vec128 c = VecCos(euler.mVec);

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

	Vec128 sign = VecConst::Sign_PNPN;
	return VecAdd(A, VecMul(B, sign));
}

inline Quat Matrix4ToQuat(const Matrix4& m)
{
	Vec128 diag;
	diag = VecAdd(VecConst::Vec_One,	VecMul(VecSwizzle1(m.mVec[0], 0), VecConst::Sign_PNNP));
	diag = VecAdd(diag,					VecMul(VecSwizzle1(m.mVec[1], 1), VecConst::Sign_NPNP));
	diag = VecAdd(diag,					VecMul(VecSwizzle1(m.mVec[2], 2), VecConst::Sign_NNPP));

	Vec128 t0 = VecShuffle(m.mVec[1], m.mVec[2], 0,2,0,1); // Y0, Y2, Z0, Z1
	Vec128 t1 = VecShuffle(m.mVec[2], m.mVec[0], 0,1,1,2); // Z0, Z1, X1, X2
	Vec128 t2 = VecShuffle(t0, t1, 1,2,2,1); // Y2, Z0, X1, Z1
	Vec128 t3 = VecShuffle(t1, t0, 1,3,0,3); // Z1, X2, Y0, Z1

	Vec128 t4 = VecAdd(t2, t3); // yz, xz, xy, ?
	Vec128 t5 = VecSub(t2, t3); // xw, yw, zw, 0
	
	Vec128 t6 = VecShuffle_0101(t4, t5); //yz, xz, xw, yw
	Vec128 t7 = VecShuffle(t4, t5, 1,0,3,2); // xz, yz, ?, zw
	Vec128 t8 = VecShuffle(t4, t6, 2,3,0,3); // xy, ?, yz, yw
	Vec128 t9 = VecShuffle(t4, t6, 3,2,1,2); // ?, xy, xz, xw

	// throughput: 3 blendconst = 1 shuffle
	Vec128 basew = VecBlendConst(t5, diag, 0,0,0,1); // xw, yw, zw, dw
	Vec128 basez = VecBlendConst(t7, diag, 0,0,1,0); // xz, yz, dz, zw
	Vec128 basey = VecBlendConst(t8, diag, 0,1,0,0); // xy, dy, yz, yw
	Vec128 basex = VecBlendConst(t9, diag, 1,0,0,0); // dx, xy, xz, xw

	Vec128 dx = VecSwizzle1(diag, 0);
	Vec128 dy = VecSwizzle1(diag, 1);
	Vec128 dz = VecSwizzle1(diag, 2);
	Vec128 dw = VecSwizzle1(diag, 3);

	// find out which one we want to use, max of (dx, dy, dz, dw)
	Vec128 cmp1 = VecCmpLT(dx, dy);
	Vec128 dv1 = VecBlend(dx, dy, cmp1);
	Vec128 base1 = VecBlend(basex, basey, cmp1);

	Vec128 cmp2 = VecCmpLT(dz, dw);
	Vec128 dv2 = VecBlend(dz, dw, cmp2);
	Vec128 base2 = VecBlend(basez, basew, cmp2);

	Vec128 cmp3 = VecCmpLT(dv1, dv2);
	Vec128 dv = VecBlend(dv1, dv2, cmp3);
	Vec128 base = VecBlend(base1, base2, cmp3);

	dv = VecDiv(VecConst::Vec_Half, VecSqrt(dv));

	return VecMul(base, dv);
}

#if 0 // this is faster but only works if w > small_num (angle != 180)
inline Quat Matrix4ToQuat(const Matrix4& m)
{
	Vec128 r;
	r = VecAdd(VecConst::Vec_One,	VecMul(VecSwizzle1(m.mVec[0], 0), VecConst::Sign_PNNP));
	r = VecAdd(r,					VecMul(VecSwizzle1(m.mVec[1], 1), VecConst::Sign_NPNP));
	r = VecAdd(r,					VecMul(VecSwizzle1(m.mVec[2], 2), VecConst::Sign_NNPP));
	r = VecSqrt(r);

	Vec128 t0 = VecShuffle(m.mVec[1], m.mVec[2], 0,2,0,1); // Y0, Y2, Z0, Z1
	Vec128 t1 = VecShuffle(m.mVec[2], m.mVec[0], 0,1,1,2); // Z0, Z1, X1, X2
	Vec128 t2 = VecShuffle(t0, t1, 1,2,2,1); // Y2, Z0, X1, Z1
	Vec128 t3 = VecShuffle(t1, t0, 1,3,0,3); // Z1, X2, Y0, Z1

	Vec128 tw = VecSub(t2, t3); // xw, yw, zw, 0
	
	Vec128 s = VecBlend(VecConst::Vec_Neg_Half, VecConst::Vec_Half, VecCmpGE(tw, VecZero()));

	return VecMul(r, s);
}
#endif

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
	t0 = VecSub(VecConst::Vec_One, t0); // 1-2zz-2yy, 1-2xx-2zz, 1-2yy-2xx, 1-4ww

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
	t0 = VecSub(VecConst::Vec_One, t0);	// 1-2zz-2yy, 1-2xx-2zz, 1-2yy-2xx, 1-4ww

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
	t0 = VecSub(VecConst::Vec_One, t0); // 1-2zz-2yy, 1-2xx-2zz, 1-2yy-2xx, 1-4ww
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
	r.mVec[3] = VecBlendConst(translation.mVec, VecConst::Vec_One, 0,0,0,1);

	return r;
}