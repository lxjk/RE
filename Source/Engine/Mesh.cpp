
#include "GL/glew.h"

#include "Render.h"

#include "Mesh.h"

std::vector<MeshData*> MeshData::gMeshDataContainer;

void MeshData::InitResource()
{
	bHasResource = true;

	// clear old buffer
	if (VAO)
		glDeleteBuffers(1, &VAO);
	if (VBO)
		glDeleteBuffers(1, &VBO);
	if (EBO)
		glDeleteBuffers(1, &EBO);

	// create buffer
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);
	// VBO data
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertCount * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
	// EBO data
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, idxCount * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
	
	// position
	{
		glEnableVertexAttribArray(Vertex::positionIdx);
		glVertexAttribPointer(Vertex::positionIdx, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
	}
	// normal
	{
		glEnableVertexAttribArray(Vertex::normalIdx);
		glVertexAttribPointer(Vertex::normalIdx, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, normal));
	}
	// tangent
	{
		glEnableVertexAttribArray(Vertex::tangentIdx);
		glVertexAttribPointer(Vertex::tangentIdx, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, tangent));
	}
	// texCoordes
	{
		glEnableVertexAttribArray(Vertex::texCoordsIdx);
		glVertexAttribPointer(Vertex::texCoordsIdx, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, texCoords));
	}

	glBindVertexArray(0);

	// calculate bounds
	for (int i = 0; i < vertCount; ++i)
		bounds += vertices[i].position.ToVector4();
}

std::vector<Mesh*> Mesh::gMeshContainer;

void Mesh::Init(MeshData* inMeshData, Material* inMaterial)
{
	meshData = inMeshData;
	material = inMaterial;

	if (meshData && !meshData->bHasResource)
		meshData->InitResource();
}

void Mesh::Draw(RenderContext& renderContext, Material* overrideMaterial) const
{
	Material* drawMaterial = overrideMaterial ? overrideMaterial : material;

	if (!meshData || !drawMaterial || !drawMaterial->shader)
		return;

	//if(renderContext.currentMaterial != material)
	drawMaterial->Use(renderContext);

	if (meshData->VAO != renderContext.currentVAO)
	{
		renderContext.currentVAO = meshData->VAO;
		glBindVertexArray(meshData->VAO);
	}
	//glDrawElements(GL_TRIANGLES, (GLsizei)meshData->indices.size(), GL_UNSIGNED_INT, 0);
	glDrawElements(GL_TRIANGLES, meshData->idxCount, GL_UNSIGNED_INT, 0);
	//glBindVertexArray(0);
}