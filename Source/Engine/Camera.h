#pragma once

#include "glm/glm.hpp"

#include "Viewpoint.h"

class Camera
{
public:
	glm::vec3 position;
	glm::vec3 euler;
	GLfloat fov;

	const Viewpoint& ProcessCamera(GLfloat width, GLfloat height, GLfloat nearPlane, GLfloat farPlane)
	{
		viewpoint.width = width;
		viewpoint.height = height;
		viewpoint.nearPlane = nearPlane;
		viewpoint.farPlane = farPlane;

		viewpoint.position = position;
		viewpoint.rotation = glm::quat(glm::radians(euler));
		viewpoint.fov = fov;

		viewpoint.CacheMatrices();

		return viewpoint;
	}

protected:
	Viewpoint viewpoint;
};