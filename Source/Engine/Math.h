#pragma once

#include "glm/glm.hpp"

#undef  PI
#define PI 					(3.1415926535897932f)
#define SMALL_NUMBER		(1.e-8f)
#define KINDA_SMALL_NUMBER	(1.e-4f)

namespace Math
{
	inline float DistSquared(const glm::vec3& v1, const glm::vec3& v2)
	{
		glm::vec3 diff = v1 - v2;
		return glm::dot(diff, diff);
	}

	inline float SizeSquared(const glm::vec3& v)
	{
		return glm::dot(v, v);
	}

	inline glm::mat3 MakeMatFromForward(const glm::vec3& f)
	{
		glm::vec3 z = -f;
		glm::vec3 up = glm::vec3(0, 1, 0);
		glm::vec3 x = glm::cross(up, z);
		if (SizeSquared(x) <= KINDA_SMALL_NUMBER)
			x = glm::vec3(1, 0, 0);
		else
			x = glm::normalize(x);
		glm::vec3 y = glm::cross(z, x);

		return glm::mat3(x, y, z);
		//return glm::mat4(glm::vec4(x, 0), glm::vec4(y, 0), glm::vec4(z, 0), glm::vec4(0));
	}

	inline glm::mat4 PerspectiveFov(float fov, float width, float height, float zNear, float zFar, float jitterX = 0, float jitterY = 0)
	{
		float w = glm::cos(0.5f * fov) / glm::sin(0.5f * fov);
		float h = w * width / height;

		glm::mat4 Result(0);
		Result[0][0] = w;
		Result[1][1] = h;
		Result[2][3] = -1;

		// negative since this is going to  * z / -z
		Result[2][0] = -jitterX * 2.f / width;
		Result[2][1] = -jitterY * 2.f / height;

		#if GLM_DEPTH_CLIP_SPACE == GLM_DEPTH_ZERO_TO_ONE
			Result[2][2] = zFar / (zNear - zFar);
			Result[3][2] = -(zFar * zNear) / (zFar - zNear);
		#else
			Result[2][2] = - (zFar + zNear) / (zFar - zNear);
			Result[3][2] = - (2 * zFar * zNear) / (zFar - zNear);
		#endif

		return Result;
	}

	inline glm::mat4 Ortho(float left, float right, float bottom, float top, float zNear, float zFar
		, float width = 1, float height = 1, float jitterX = 0, float jitterY = 0)
	{
		glm::mat4 Result(1);
		Result[0][0] = 2.f / (right - left);
		Result[1][1] = 2.f / (top - bottom);
		Result[3][0] = -(right + left) / (right - left) + 2 * jitterX / width;
		Result[3][1] = -(top + bottom) / (top - bottom) + 2 * jitterY / height;

		#if GLM_DEPTH_CLIP_SPACE == GLM_DEPTH_ZERO_TO_ONE
			Result[2][2] = -1.f / (zFar - zNear);
			Result[3][2] = -zNear / (zFar - zNear);
		#else
			Result[2][2] = -2.f / (zFar - zNear);
			Result[3][2] = -(zFar + zNear) / (zFar - zNear);
		#endif

		return Result;
	}
}