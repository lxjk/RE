#pragma once

#include "glm/glm.hpp"

class Light
{
public:

	glm::vec3 position;
	glm::vec3 direction;
	glm::vec3 color;
	float intensity;
	float radius;

	Light()
	{
		position = glm::vec3(0, 0, 0);
		direction = glm::vec3(0, 0, -1);
		color = glm::vec3(0, 0, 0);
		intensity = 0;
		radius = 1;
	}

	glm::vec3 GetColorIntensity()
	{
		return color * intensity;
	}

	glm::vec3 GetPositionViewSpace(const glm::mat4& viewMat)
	{
		return glm::vec3(viewMat * glm::vec4(position, 1.f));
	}

	glm::vec3 GetDirectionViewSpace(const glm::mat4& viewMat)
	{
		return glm::mat3(viewMat) * direction;
	}
};