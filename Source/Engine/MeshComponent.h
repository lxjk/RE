#pragma once

#include <vector>

#include "glm/glm.hpp"

#include "Component.h"

class Mesh;

class MeshComponent : public Component
{
public:
	static std::vector<MeshComponent*> gMeshComponentContainer;

	static MeshComponent* Create()
	{
		MeshComponent* mc = new MeshComponent();
		gMeshComponentContainer.push_back(mc);
		return mc;
	}

	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;

	glm::mat4 modelMat;
	glm::mat3 normalMat;

	std::vector<Mesh*> meshList;

	bool bRenderTransformDirty;

	MeshComponent()
	: position(glm::vec3(0))
	, rotation(glm::vec3(0))
	, scale(glm::vec3(1))
	, bRenderTransformDirty(true)
	{}

	MeshComponent(const std::vector<Mesh*>& inMeshList,
		glm::vec3 inPosition = glm::vec3(0),
		glm::vec3 inRotation = glm::vec3(0),
		glm::vec3 inScale = glm::vec3(1))
		: meshList(inMeshList)
		, position(inPosition)
		, rotation(inRotation)
		, scale(inScale)
		, bRenderTransformDirty(true)
	{}

	inline void SetPosition(const glm::vec3& inPosition)
	{
		position = inPosition;
		bRenderTransformDirty = true;
	}

	inline void SetRotation(const glm::vec3& inRotation)
	{
		rotation = inRotation;
		bRenderTransformDirty = true;
	}

	inline void SetScale(const glm::vec3& inScale)
	{
		scale = inScale;
		bRenderTransformDirty = true;
	}


	virtual void UpdateEndOfFrame(float deltaTime) override;

	void Draw(struct RenderContext& renderContext, Material* overrideMaterial = 0);

protected:
	void CacheRenderMatrices();
};