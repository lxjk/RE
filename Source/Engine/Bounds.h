#pragma once

#include "../Math/REMath.h"

class BoxBounds
{
public:
	Vector4_3 min;
	Vector4_3 max;

	BoxBounds()
	: min(Vector4_3(FLT_MAX))
	, max(Vector4_3(-FLT_MAX))
	{}

	inline Vector4_3 GetCenter()
	{
		return (min + max) * 0.5f;
	}

	inline Vector4_3 GetExtent()
	{
		return (max - min) * 0.5f;
	}

	inline bool IsInBounds(const Vector4_3& point)
	{
		Vec128 t0 = VecCmpLE(point.mVec, max.mVec);
		Vec128 t1 = VecCmpGE(point.mVec, min.mVec);
		int r = VecMoveMask(VecAnd(t0, t1));
		return (r & 0x7) == 0x7; // ignore w component
		//return point.x <= max.x && point.x >= min.x &&
		//	point.y <= max.y && point.y >= min.y &&
		//	point.z <= max.z && point.z >= min.z;
	}

	inline bool IsOverlap(const BoxBounds& otherBounds)
	{
		Vec128 t0 = VecCmpLE(otherBounds.min.mVec, max.mVec);
		Vec128 t1 = VecCmpGE(otherBounds.max.mVec, min.mVec);
		int r = VecMoveMask(VecAnd(t0, t1));
		return (r & 0x7) == 0x7; // ignore w component
		//return otherBounds.max.x >= min.x && otherBounds.max.y >= min.y && otherBounds.max.z >= min.z &&
		//	otherBounds.min.x <= max.x && otherBounds.min.y <= max.y && otherBounds.min.z <= max.z;
	}

	void TransformBounds(const Matrix4& inMat, BoxBounds& outBounds)
	{
		//Vector4_3 boundsPoints[8];
		//GetPoints(boundsPoints);
		//for (int pIdx = 0; pIdx < 8; ++pIdx)
		//{
		//	outBounds += inMat.TransformPoint(boundsPoints[pIdx]);
		//}
		outBounds += inMat.TransformPoint(min);
		outBounds += inMat.TransformPoint(max);
		outBounds += inMat.TransformPoint(VecBlend(min.mVec, max.mVec, 0,0,1,0));
		outBounds += inMat.TransformPoint(VecBlend(min.mVec, max.mVec, 0,1,0,0));
		outBounds += inMat.TransformPoint(VecBlend(min.mVec, max.mVec, 1,0,0,0));
		outBounds += inMat.TransformPoint(VecBlend(min.mVec, max.mVec, 0,1,1,0));
		outBounds += inMat.TransformPoint(VecBlend(min.mVec, max.mVec, 1,0,1,0));
		outBounds += inMat.TransformPoint(VecBlend(min.mVec, max.mVec, 1,1,0,0));
	}

	// we don't do check here, expect an array of at least 8 elements
	void GetPoints(Vector4_3* outPoints)
	{
		outPoints[0] = min;
		outPoints[1] = max;
		outPoints[2] = VecBlend(min.mVec, max.mVec, 0,0,1,0);
		outPoints[3] = VecBlend(min.mVec, max.mVec, 0,1,0,0);
		outPoints[4] = VecBlend(min.mVec, max.mVec, 1,0,0,0);
		outPoints[5] = VecBlend(min.mVec, max.mVec, 0,1,1,0);
		outPoints[6] = VecBlend(min.mVec, max.mVec, 1,0,1,0);
		outPoints[7] = VecBlend(min.mVec, max.mVec, 1,1,0,0);
	}

	BoxBounds& operator+= (const Vector4_3& point)
	{
		min.mVec = VecMin(min.mVec, point.mVec);
		max.mVec = VecMax(max.mVec, point.mVec);
		//if (point.x < min.x) min.x = point.x;
		//if (point.y < min.y) min.y = point.y;
		//if (point.z < min.z) min.z = point.z;
		//if (point.x > max.x) max.x = point.x;
		//if (point.y > max.y) max.y = point.y;
		//if (point.z > max.z) max.z = point.z;
		return *this;
	}

	BoxBounds& operator+= (const BoxBounds& otherBounds)
	{
		min.mVec = VecMin(min.mVec, otherBounds.min.mVec);
		max.mVec = VecMax(max.mVec, otherBounds.max.mVec);
		//if (otherBounds.min.x < min.x) min.x = otherBounds.min.x;
		//if (otherBounds.min.y < min.y) min.y = otherBounds.min.y;
		//if (otherBounds.min.z < min.z) min.z = otherBounds.min.z;
		//if (otherBounds.max.x > max.x) max.x = otherBounds.max.x;
		//if (otherBounds.max.y > max.y) max.y = otherBounds.max.y;
		//if (otherBounds.max.z > max.z) max.z = otherBounds.max.z;
		return *this;
	}
};