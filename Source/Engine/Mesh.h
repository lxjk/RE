#pragma once

#include "GL/glew.h"
#include "glm/glm.hpp"

#include <vector>

#include "Shader.h"

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec2 texCoords;

	Vertex(glm::vec3 inPosition,
		glm::vec3 inNormal,
		glm::vec3 inTangent,
		glm::vec2 inTexCoords)
	{
		position = inPosition;
		normal = inNormal;
		tangent = inTangent;
		texCoords = inTexCoords;
	}
};

class MeshData
{
public:
	std::vector<Vertex> vertices;
	std::vector<GLuint> indices;
};

class Mesh
{
public:

	Mesh() : 
	  VAO(0)
	, VBO(0)
	, EBO(0)
	{}

	void Init(MeshData* inMeshData, Shader* inShader)
	{
		meshData = inMeshData;
		shader = inShader;

		InitResource();
	}

	void Draw()
	{
		if (!meshData && !shader)
			return;

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, (GLsizei)meshData->indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(VAO);
	}

	void InitResource()
	{
		if (!meshData && !shader)
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

		// set attributes
		if (shader->positionIdx >= 0)
		{
			glEnableVertexAttribArray(shader->positionIdx);
			glVertexAttribPointer(shader->positionIdx, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
		}
		if (shader->normalIdx >= 0)
		{
			glEnableVertexAttribArray(shader->normalIdx);
			glVertexAttribPointer(shader->normalIdx, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, normal));
		}
		if (shader->tangentIdx >= 0)
		{
			glEnableVertexAttribArray(shader->tangentIdx);
			glVertexAttribPointer(shader->tangentIdx, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, tangent));
		}
		if (shader->texCoordsIdx >= 0)
		{
			glEnableVertexAttribArray(shader->texCoordsIdx);
			glVertexAttribPointer(shader->texCoordsIdx, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, texCoords));
		}

		glBindVertexArray(0);
	}

protected:
	MeshData* meshData;
	Shader* shader;

	GLuint VAO, VBO, EBO;
};