#include "Render.h"

#include "Mesh.h"

#include "MeshComponent.h"

std::vector<MeshComponent*> MeshComponent::gMeshComponentContainer;

void MeshComponent::CacheRenderMatrices()
{
	bRenderTransformDirty = false;

	prevModelMat = modelMat;
	modelMat = MakeMatrix(position, EulerToQuat(rotation), scale);

	if (!bHasCachedRenderTransform)
	{
		bHasCachedRenderTransform = true;
		prevModelMat = modelMat;
	}
}

void MeshComponent::UpdateEndOfFrame(float deltaTime)
{
	if (bRenderTransformDirty)
		CacheRenderMatrices();
}

void MeshComponent::SetMeshList(const std::vector<Mesh*>& inMeshList)
{
	meshList = inMeshList;
	for (int i = 0, ni = (int)inMeshList.size(); i < ni; ++i)
	{
		MeshData* meshData = inMeshList.data()[i]->meshData;
		if(meshData)
			bounds += meshData->bounds;
	}
}

void MeshComponent::AddMesh(Mesh* inMesh)
{
	meshList.push_back(inMesh);
	if(inMesh->meshData)
		bounds += inMesh->meshData->bounds;
}

void MeshComponent::Draw(RenderContext& renderContext, Material* overrideMaterial)
{
	if (overrideMaterial)
	{
		overrideMaterial->SetParameter("prevModelMat", prevModelMat);
		overrideMaterial->SetParameter("modelMat", modelMat);
		//overrideMaterial->SetParameter("normalMat", normalMat);
	}
	Mesh** meshListPtr = meshList.data();
	for (int i = 0, ni = (int)meshList.size(); i < ni; ++i)
	{
		if (!overrideMaterial)
		{
			meshListPtr[i]->material->SetParameter("prevModelMat", prevModelMat);
			meshListPtr[i]->material->SetParameter("modelMat", modelMat);
			//meshListPtr[i]->material->SetParameter("normalMat", normalMat);
		}

		meshListPtr[i]->Draw(renderContext, overrideMaterial);
	}
}