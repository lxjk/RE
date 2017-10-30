#include "Render.h"

#include "Mesh.h"

#include "MeshComponent.h"

REArray<MeshComponent*> MeshComponent::gMeshComponentContainer;

void MeshComponent::CacheRenderMatrices()
{
	bRenderTransformDirty = false;

	prevModelMat = modelMat;
	modelMat = QuatToMatrix4(EulerToQuat(rotation));
	modelMat.SetTranslation(position);
	invModelMatNoScale = modelMat.GetTransformInverseNoScale();
	modelMat.ApplyScale(scale);
	//modelMat = MakeMatrix(position, EulerToQuat(rotation), scale);

	scaledBounds.min = bounds.min * scale;
	scaledBounds.max = bounds.max * scale;

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

void MeshComponent::SetMeshList(const REArray<Mesh*>& inMeshList)
{
	meshList = inMeshList;
	for (int i = 0, ni = (int)inMeshList.size(); i < ni; ++i)
	{
		MeshData* meshData = inMeshList[i]->meshData;
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
	for (int i = 0, ni = (int)meshList.size(); i < ni; ++i)
	{
		Mesh*& mesh = meshList[i];
		if (!overrideMaterial)
		{
			mesh->material->SetParameter("prevModelMat", prevModelMat);
			mesh->material->SetParameter("modelMat", modelMat);
			//meshListPtr[i]->material->SetParameter("normalMat", normalMat);
		}

		mesh->Draw(renderContext, overrideMaterial);
	}
}