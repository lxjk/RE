#pragma once

// opengl
#include "SDL_opengl.h"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_inverse.hpp"

#include "Math.h"

class Viewpoint
{
public:
	glm::vec3 position;
	glm::quat rotation;
	float fov;
	float width;
	float height;
	float nearPlane;
	float farPlane;
	float jitterX; // in pixel scale
	float jitterY; // in pixel scale

	float nearRadius;

	glm::mat4 viewMat;
	glm::mat4 invViewMat;
	glm::mat4 projMat;
	glm::mat4 viewProjMat;
	
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
		//projMat = glm::perspectiveFov(fov / width * height, width, height, nearPlane, farPlane);
		projMat = Math::PerspectiveFov(fov, width, height, nearPlane, farPlane, jitterX, jitterY);

		viewProjMat = projMat * viewMat;

		float tanHalfFOV = glm::tan(fov * 0.5f);
		nearRadius = nearPlane * sqrt(1 + tanHalfFOV * tanHalfFOV * (1 + height * height / width / width));
	}

	// we don't do bound check here, expect an array of at least 4 elements
	void GetClipPoints(float z, glm::vec3* outPoints)
	{
		float x = z / projMat[0][0];
		float y = z / projMat[1][1];
		outPoints[0] = invViewMat * glm::vec4(-x, y, z, 1.f);
		outPoints[1] = invViewMat * glm::vec4(-x, -y, z, 1.f);
		outPoints[2] = invViewMat * glm::vec4(x, -y, z, 1.f);
		outPoints[3] = invViewMat * glm::vec4(x, y, z, 1.f);
	}
};