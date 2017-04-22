#pragma once

#include "glm/glm.hpp"

#include "Mesh.h"
#include "Material.h"

#define MAX_CASCADE_COUNT 3

struct ShadowData
{
	Texture2D* shadowMap;
	glm::vec3 bounds; // x cascade width, y cascade height, z far plane

	// this is remap * lightProj * lightView * invCameraView, which converts VS -> WS -> LS -> [-1, 1] -> [0, 1]
	glm::mat4 shadowMat;
};

class Light
{
public:

	glm::vec3 position;
	glm::vec3 direction;
	glm::vec3 color;
	float intensity;
	float radius;
	float outerHalfAngle;
	float innerHalfAngle;

	glm::vec3 colorIntensity;
	float invRadius;
	float radialAttenuationBlend;
	float outerCosHalfAngle;
	float invDiffCosHalfAngle;
	float endRadius;

	glm::vec4 attenParams; // bRadial, bSpot, outerCosHalfAngle, invDiffCosHalfAngle

	glm::mat4 modelMat;
	glm::mat4 lightViewMat;

	ShadowData shadowData[MAX_CASCADE_COUNT];

	Mesh* LightMesh;

	Light(Mesh* inLightMesh)
	{
		LightMesh = inLightMesh;
		position = glm::vec3(0, 0, 0);
		direction = glm::vec3(0, 0, -1);
		color = glm::vec3(0, 0, 0);
		intensity = 0;
		radius = 1;
		invRadius = 1;
		radialAttenuationBlend = 0;
		outerCosHalfAngle = 0;
		invDiffCosHalfAngle = 1;
		attenParams = glm::vec4(0, 0, 0, 1);
	}

	void SetRadius(float inRadius)
	{
		radius = inRadius;
		if (radius > KINDA_SMALL_NUMBER)
		{
			invRadius = 1.f / radius;
			radialAttenuationBlend = glm::clamp((radius - 2.5f) / 9.f, 0.f, 1.f);
		}
	}

	void BuildModelMat()
	{
		if (attenParams.y > 0)
		{
			// spot light
			modelMat = Math::MakeMatFromForward(direction);
			modelMat[3] = glm::vec4(position, 1);
			modelMat = glm::scale(modelMat, glm::vec3(endRadius, endRadius, radius));

			lightViewMat = glm::transpose(Math::MakeMatFromForward(direction));
			lightViewMat = glm::translate(lightViewMat, -position);
		}
		else if (attenParams.x > 0)
		{
			// point light
			modelMat = glm::mat4(1);
			modelMat = glm::translate(modelMat, position);
			modelMat = glm::scale(modelMat, glm::vec3(radius));

			lightViewMat = glm::mat4(1);
			lightViewMat[3] = glm::vec4(-position, 1.f);
		}
		else
		{
			lightViewMat = glm::transpose(Math::MakeMatFromForward(direction));
		}
	}

	void SetDirectionLight(glm::vec3 inDir, glm::vec3 inColor, float inIntensity)
	{
		direction = glm::normalize(inDir);
		color = inColor;
		intensity = inIntensity;

		colorIntensity = color * intensity;

		attenParams.x = 0;
		attenParams.y = 0;
		attenParams.z = outerCosHalfAngle;
		attenParams.w = invDiffCosHalfAngle;

		BuildModelMat();

		//LightMesh->material->SetParameter(ShaderNameBuilder("light")("color").c_str(), colorIntensity);
		//LightMesh->material->SetParameter(ShaderNameBuilder("light")("attenParams").c_str(), attenParams);
	}

	void SetPointLight(glm::vec3 inPos, float inRadius, glm::vec3 inColor, float inIntensity)
	{
		position = inPos;
		color = inColor;
		intensity = inIntensity;

		SetRadius(inRadius);
		
		colorIntensity = color * intensity;

		attenParams.x = 1;
		attenParams.y = 0;
		attenParams.z = outerCosHalfAngle;
		attenParams.w = invDiffCosHalfAngle;

		BuildModelMat();

		LightMesh->material->SetParameter(ShaderNameBuilder("light")("color").c_str(), colorIntensity);
		LightMesh->material->SetParameter(ShaderNameBuilder("light")("attenParams").c_str(), attenParams);
	}

	void SetSpotLight(glm::vec3 inPos, glm::vec3 inDir, float inRadius, float inOuterHalfAngle, float inInnerHalfAngle, glm::vec3 inColor, float inIntensity)
	{
		position = inPos;
		direction = glm::normalize(inDir);
		color = inColor;
		intensity = inIntensity;
		outerHalfAngle = inOuterHalfAngle;
		innerHalfAngle = inInnerHalfAngle;

		SetRadius(inRadius);

		colorIntensity = color * intensity;
		float outerRad = glm::radians(outerHalfAngle);
		float innerRad = glm::radians(innerHalfAngle);
		outerCosHalfAngle = glm::cos(outerRad);
		invDiffCosHalfAngle = 1.f / (glm::cos(innerRad) - outerCosHalfAngle);
		endRadius = radius * glm::tan(outerRad);

		attenParams.x = 1;
		attenParams.y = 1;
		attenParams.z = outerCosHalfAngle;
		attenParams.w = invDiffCosHalfAngle;

		BuildModelMat();

		LightMesh->material->SetParameter(ShaderNameBuilder("light")("color").c_str(), colorIntensity);
		LightMesh->material->SetParameter(ShaderNameBuilder("light")("attenParams").c_str(), attenParams);
	}

	void SetDirection(const glm::vec3& inDirection)
	{
		direction = inDirection;
		BuildModelMat();
	}

	glm::vec4 GetPositionVSInvR(const glm::mat4& viewMat) const
	{
		glm::vec4 result = viewMat * glm::vec4(position, 1.f);
		result.w = invRadius;
		return result;
	}

	glm::vec4 GetDirectionVSRAB(const glm::mat4& viewMat) const
	{
		return glm::vec4(glm::mat3(viewMat) * direction, radialAttenuationBlend);
	}
};