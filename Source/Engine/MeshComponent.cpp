
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "glm/gtc/matrix_inverse.hpp"

#include "Render.h"

#include "Mesh.h"

#include "MeshComponent.h"

std::vector<MeshComponent*> MeshComponent::gMeshComponentContainer;

void MeshComponent::CacheRenderMatrices()
{
	bRenderTransformDirty = false;

	//modelMat = glm::orientate4(rotation);
	modelMat = glm::mat4_cast(glm::quat(glm::radians(rotation)));
	modelMat = glm::scale(modelMat, scale);
	modelMat[3] = glm::vec4(position, 1.f);
	normalMat = glm::inverseTranspose(glm::mat3(modelMat));
}

void MeshComponent::UpdateEndOfFrame(float deltaTime)
{
	if (bRenderTransformDirty)
		CacheRenderMatrices();
}

void MeshComponent::Draw(RenderContext& renderContext, Material* overrideMaterial)
{
	if (overrideMaterial)
	{
		overrideMaterial->SetParameter("modelMat", modelMat);
		overrideMaterial->SetParameter("normalMat", normalMat);
	}
	Mesh** meshListPtr = meshList.data();
	for (int i = 0, ni = meshList.size(); i < ni; ++i)
	{
		if (!overrideMaterial)
		{
			meshListPtr[i]->material->SetParameter("modelMat", modelMat);
			meshListPtr[i]->material->SetParameter("normalMat", normalMat);
		}

		meshListPtr[i]->Draw(renderContext, overrideMaterial);
	}
}