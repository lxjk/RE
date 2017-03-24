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
	float outerHalfAngle;
	float innerHalfAngle;

	glm::vec3 colorIntensity;
	float invRadius;
	float outerCosHalfAngle;
	float invDiffCosHalfAngle;
	float endRadius;

	glm::vec4 attenParams; // bRadial, bSpot, outerCosHalfAngle, invDiffCosHalfAngle

	Light()
	{
		position = glm::vec3(0, 0, 0);
		direction = glm::vec3(0, 0, -1);
		color = glm::vec3(0, 0, 0);
		intensity = 0;
		radius = 1;
		invRadius = 1;
		outerCosHalfAngle = 0;
		invDiffCosHalfAngle = 1;
		attenParams = glm::vec4(0, 0, 0, 1);
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
	}

	void SetPointLight(glm::vec3 inPos, float inRadius, glm::vec3 inColor, float inIntensity)
	{
		position = inPos;
		radius = inRadius;
		color = inColor;
		intensity = inIntensity;
		
		colorIntensity = color * intensity;
		invRadius = 1.f / radius;

		attenParams.x = 1;
		attenParams.y = 0;
		attenParams.z = outerCosHalfAngle;
		attenParams.w = invDiffCosHalfAngle;
	}

	void SetSpotLight(glm::vec3 inPos, glm::vec3 inDir, float inRadius, float inOuterHalfAngle, float inInnerHalfAngle, glm::vec3 inColor, float inIntensity)
	{
		position = inPos;
		direction = glm::normalize(inDir);
		radius = inRadius;
		color = inColor;
		intensity = inIntensity;
		outerHalfAngle = inOuterHalfAngle;
		innerHalfAngle = inInnerHalfAngle;

		colorIntensity = color * intensity;
		invRadius = 1.f / radius;
		float outerRad = glm::radians(outerHalfAngle);
		float innerRad = glm::radians(innerHalfAngle);
		outerCosHalfAngle = glm::cos(outerRad);
		invDiffCosHalfAngle = 1.f / (glm::cos(innerRad) - outerCosHalfAngle);
		endRadius = radius * glm::tan(outerRad);

		attenParams.x = 1;
		attenParams.y = 1;
		attenParams.z = outerCosHalfAngle;
		attenParams.w = invDiffCosHalfAngle;
	}

	glm::vec4 GetPositionVSInvR(const glm::mat4& viewMat) const
	{
		glm::vec4 result = viewMat * glm::vec4(position, 1.f);
		result.w = invRadius;
		return result;
	}

	glm::vec3 GetDirectionVS(const glm::mat4& viewMat) const
	{
		return glm::mat3(viewMat) * direction;
	}
};