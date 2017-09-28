#pragma once

#include <vector>

#include "Math/REMath.h"

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

	Vector4_3 position;
	Vector4_3 rotation; // degrees
	Vector4_3 scale;

	Matrix4 prevModelMat;
	Matrix4 modelMat;


	BoxBounds bounds;

	BoxBounds boundsLS;

	MeshComponent()
	: position(Vector4_3::Zero())
	, rotation(Vector4_3::Zero())
	, scale(Vector4_3(1))
	, bRenderTransformDirty(true)
	{}

	MeshComponent(const std::vector<Mesh*>& inMeshList,
		Vector4_3 inPosition = Vector4_3::Zero(),
		Vector4_3 inRotation = Vector4_3::Zero(),
		Vector4_3 inScale = Vector4_3(1))
		: position(inPosition)
		, rotation(inRotation)
		, scale(inScale)
		, bRenderTransformDirty(true)
		, bHasCachedRenderTransform(false)
	{
		SetMeshList(inMeshList);
	}

	inline void SetPosition(const Vector4_3& inPosition)
	{
		position = inPosition;
		bRenderTransformDirty = true;
	}

	inline void SetRotation(const Vector4_3& inRotation)
	{
		rotation = inRotation;
		bRenderTransformDirty = true;
	}

	inline void SetScale(const Vector4_3& inScale)
	{
		scale = inScale;
		bRenderTransformDirty = true;
	}


	virtual void UpdateEndOfFrame(float deltaTime) override;

	const std::vector<Mesh*>& GetMeshList() { return meshList; }
	void SetMeshList(const std::vector<Mesh*>& inMeshList);
	void AddMesh(Mesh* inMesh);

	void Draw(struct RenderContext& renderContext, Material* overrideMaterial = 0);

protected:

	std::vector<Mesh*> meshList;
	bool bRenderTransformDirty;
	bool bHasCachedRenderTransform;

	void CacheRenderMatrices();
};