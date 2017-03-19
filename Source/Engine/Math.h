#pragma once

#include "glm/glm.hpp"

#undef  PI
#define PI 					(3.1415926535897932f)
#define SMALL_NUMBER		(1.e-8f)
#define KINDA_SMALL_NUMBER	(1.e-4f)

namespace Math
{
	inline float DistSquared(const glm::vec3& v1, const glm::vec3 v2)
	{
		glm::vec3 diff = v1 - v2;
		return glm::dot(diff, diff);
	}
}