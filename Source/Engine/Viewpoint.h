#pragma once

// opengl
#include "SDL_opengl.h"

#include "Math/REMath.h"

class Viewpoint
{
public:
	Vector4_3 position;
	Quat rotation;
	float fov; // radian
	float width;
	float height;
	float nearPlane;
	float farPlane;
	float jitterX; // in pixel scale
	float jitterY; // in pixel scale

	float nearRadius;
	float screenScale; // use to calculate pixel size of a sphere on screen

	Matrix4 viewMat;
	Matrix4 invViewMat;
	Matrix4 projMat;
	Matrix4 viewProjMat;

	Plane frustumPlanes[6];
	
	void CacheMatrices()
	{
		// invView
		// rotation
		invViewMat = QuatToMatrix4(rotation);
		// translation
		invViewMat.SetTranslation(position);
		// convert
		invViewMat = ToInvViewMatrix(invViewMat);
		// view
		viewMat = invViewMat.GetTransformInverseNoScale();
		// proj
		//projMat = PerspectiveFov(fov / width * height, width, height, nearPlane, farPlane);
		projMat = MakeMatrixPerspectiveProj(fov, width, height, nearPlane, farPlane, jitterX, jitterY);

		viewProjMat = projMat * viewMat;

		float tanHalfFOV = Tan(fov * 0.5f);
		nearRadius = nearPlane * Sqrt(1 + tanHalfFOV * tanHalfFOV * (1 + height * height / width / width));

		screenScale = Max(projMat.m[0][0] * 0.5f * width, projMat.m[1][1] * 0.5f * height);
		
		GetFrustumPlanes(viewMat, projMat.m[0][0], projMat.m[1][1], nearPlane, farPlane, frustumPlanes);
	}

	// we don't do bound check here, expect an array of at least 4 elements
	void GetClipPoints(float z, Vector4_3* outPoints)
	{
		float x = z / projMat.m[0][0];
		float y = z / projMat.m[1][1];
		outPoints[0] = invViewMat.TransformPoint(Vector4_3(-x, y, z));
		outPoints[1] = invViewMat.TransformPoint(Vector4_3(-x, -y, z));
		outPoints[2] = invViewMat.TransformPoint(Vector4_3(x, -y, z));
		outPoints[3] = invViewMat.TransformPoint(Vector4_3(x, y, z));
	}
};