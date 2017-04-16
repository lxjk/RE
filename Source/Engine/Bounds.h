#pragma once

#include "glm/glm.hpp"

class BoxBounds
{
public:
	glm::vec3 min;
	glm::vec3 max;

	BoxBounds()
	: min(glm::vec3(FLT_MAX))
	, max(glm::vec3(-FLT_MAX))
	{}

	inline glm::vec3 GetCenter()
	{
		return (min + max) * 0.5f;
	}

	inline glm::vec3 GetExtent()
	{
		return (max - min) * 0.5f;
	}

	inline bool IsInBounds(const glm::vec3& point)
	{
		return point.x <= max.x && point.x >= min.x &&
			point.y <= max.y && point.y >= min.y &&
			point.z <= max.z && point.z >= min.z;
	}

	inline bool IsOverlap(const BoxBounds& otherBounds)
	{
		return otherBounds.max.x >= min.x && otherBounds.max.y >= min.y && otherBounds.max.z >= min.z &&
			otherBounds.min.x <= max.x && otherBounds.min.y <= max.y && otherBounds.min.z <= max.z;
	}

	void TransformBounds(const glm::mat4& inMat, BoxBounds& outBounds)
	{
		glm::vec3 boundsPoints[8];
		GetPoints(boundsPoints);
		for (int pIdx = 0; pIdx < 8; ++pIdx)
		{
			outBounds += inMat * glm::vec4(boundsPoints[pIdx], 1);
		}
	}

	// we don't do check here, expect an array of at least 8 elements
	void GetPoints(glm::vec3* outPoints)
	{
		outPoints[0] = min;
		outPoints[1] = max;
		outPoints[2] = glm::vec3(min.x, min.y, max.z);
		outPoints[3] = glm::vec3(min.x, max.y, min.z);
		outPoints[4] = glm::vec3(max.x, min.y, min.z);
		outPoints[5] = glm::vec3(min.x, max.y, max.z);
		outPoints[6] = glm::vec3(max.x, min.y, max.z);
		outPoints[7] = glm::vec3(max.x, max.y, min.z);
	}

	BoxBounds& operator+= (const glm::vec3& point)
	{
		if (point.x < min.x) min.x = point.x;
		if (point.y < min.y) min.y = point.y;
		if (point.z < min.z) min.z = point.z;
		if (point.x > max.x) max.x = point.x;
		if (point.y > max.y) max.y = point.y;
		if (point.z > max.z) max.z = point.z;
		return *this;
	}

	BoxBounds& operator+= (const BoxBounds& otherBounds)
	{
		if (otherBounds.min.x < min.x) min.x = otherBounds.min.x;
		if (otherBounds.min.y < min.y) min.y = otherBounds.min.y;
		if (otherBounds.min.z < min.z) min.z = otherBounds.min.z;
		if (otherBounds.max.x > max.x) max.x = otherBounds.max.x;
		if (otherBounds.max.y > max.y) max.y = otherBounds.max.y;
		if (otherBounds.max.z > max.z) max.z = otherBounds.max.z;
		return *this;
	}
};