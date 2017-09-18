#pragma once

#include "Viewpoint.h"

class Camera
{
public:
	Vector4_3 position;
	Vector4_3 euler; // degree
	float fov; // degree

	const Viewpoint& ProcessCamera(float width, float height, float nearPlane, float farPlane, float jitterX = 0, float jitterY = 0)
	{
		viewpoint.width = width;
		viewpoint.height = height;
		viewpoint.nearPlane = nearPlane;
		viewpoint.farPlane = farPlane;
		viewpoint.jitterX = jitterX;
		viewpoint.jitterY = jitterY;

		viewpoint.position = position;
		viewpoint.rotation = EulerToQuat(euler);
		viewpoint.fov = DegToRad(fov);

		viewpoint.CacheMatrices();

		return viewpoint;
	}

protected:
	Viewpoint viewpoint;
};