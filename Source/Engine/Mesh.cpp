
#include "GL/glew.h"

#include "Render.h"

#include "Mesh.h"

std::vector<MeshData*> MeshData::gMeshDataContainer;

std::vector<Mesh*> Mesh::gMeshContainer;

void Mesh::Init(MeshData* inMeshData, Material* inMaterial)
{
	meshData = inMeshData;
	material = inMaterial;

	InitResource();
}

void Mesh::Draw(RenderContext& renderContext) const
{
	if (!meshData || !material || !material->shader)
		return;

	//if(renderContext.currentMaterial != material)
		material->Use(renderContext);

	glBindVertexArray(VAO);
	//glDrawElements(GL_TRIANGLES, (GLsizei)meshData->indices.size(), GL_UNSIGNED_INT, 0);
	glDrawElements(GL_TRIANGLES, meshData->idxCount, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void Mesh::InitResource()
{
	if (!meshData || !material || !material->shader)
		return;

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
	glBufferData(GL_ARRAY_BUFFER, meshData->vertices.size() * sizeof(Vertex), meshData->vertices.data(), GL_STATIC_DRAW);
	// EBO data
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshData->indices.size() * sizeof(GLuint), meshData->indices.data(), GL_STATIC_DRAW);

	glBindVertexArray(0);

	SetAttributes();

}

void Mesh::SetAttributes()
{
	if (VAO == 0)
		return;

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	// set attributes
	if (material->shader->positionIdx >= 0)
	{
		glEnableVertexAttribArray(material->shader->positionIdx);
		glVertexAttribPointer(material->shader->positionIdx, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
	}
	if (material->shader->normalIdx >= 0)
	{
		glEnableVertexAttribArray(material->shader->normalIdx);
		glVertexAttribPointer(material->shader->normalIdx, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, normal));
	}
	if (material->shader->tangentIdx >= 0)
	{
		glEnableVertexAttribArray(material->shader->tangentIdx);
		glVertexAttribPointer(material->shader->tangentIdx, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, tangent));
	}
	if (material->shader->texCoordsIdx >= 0)
	{
		glEnableVertexAttribArray(material->shader->texCoordsIdx);
		glVertexAttribPointer(material->shader->texCoordsIdx, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, texCoords));
	}

	glBindVertexArray(0);
}