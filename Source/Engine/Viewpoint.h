#pragma once

// opengl
#include "SDL_opengl.h"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_inverse.hpp"

class Viewpoint
{
public:
	glm::vec3 position;
	glm::quat rotation;
	GLfloat fov;
	GLfloat width;
	GLfloat height;
	GLfloat nearPlane;
	GLfloat farPlane;

	glm::mat4 viewMat;
	glm::mat4 invViewMat;
	glm::mat4 projMat;

	void CacheMatrices()
	{
		// invView
		// rotation
		invViewMat = glm::mat4_cast(rotation);
		// translation
		invViewMat[3] = glm::vec4(position, 1.f);
		// view
		// invert rotation
		viewMat = glm::transpose(glm::mat3_cast(rotation));
		// invert translation
		//viewMat[3] = viewMat * glm::vec4((-position), 1.f);
		viewMat = glm::translate(viewMat, -position);
		// proj
		projMat = glm::perspectiveFov(fov / width * height, width, height, nearPlane, farPlane);
	}
};