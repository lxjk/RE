#pragma once

#include "GL/glew.h"
#include "glm/glm.hpp"

#include <vector>

#include "Math.h"
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

	void Draw() const
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


void MakeCube(MeshData& meshData)
{
	std::vector<Vertex>& vertList = meshData.vertices;
	std::vector<GLuint>& idxList = meshData.indices;
	glm::vec2 uv[4] =
	{
		glm::vec2(0, 0),
		glm::vec2(0, 1),
		glm::vec2(1, 1),
		glm::vec2(1, 0)
	};
	glm::vec3 normal[6] =
	{
		glm::vec3(1, 0, 0),
		glm::vec3(0, 1, 0),
		glm::vec3(0, 0, 1),
		glm::vec3(-1, 0, 0),
		glm::vec3(0, -1, 0),
		glm::vec3(0, 0, -1),
	};
	glm::vec3 up[6] =
	{
		glm::vec3(0, 1, 0),
		glm::vec3(0, 0, -1),
		glm::vec3(0, 1, 0),
		glm::vec3(0, 1, 0),
		glm::vec3(0, 0, 1),
		glm::vec3(0, 1, 0),
	};
	vertList.reserve(24);
	idxList.reserve(36);
	// +x -> +y -> +z -> -x -> -y -> -z
	for (int i = 0; i < 6; ++i)
	{
		glm::vec3 right = glm::cross(up[i], normal[i]);
		for (int j = 0; j < 4; ++j)
		{
			glm::vec2 ext = uv[j] * 2.f - 1.f;
			vertList.push_back(Vertex(normal[i] + ext.x * right - ext.y * up[i], normal[i], right, uv[j]));
		}
		idxList.push_back(i * 4 + 0);
		idxList.push_back(i * 4 + 1);
		idxList.push_back(i * 4 + 2);
		idxList.push_back(i * 4 + 0);
		idxList.push_back(i * 4 + 2);
		idxList.push_back(i * 4 + 3);
	}
}

void MakeSphere(MeshData& meshData, int div)
{
	std::vector<Vertex>& vertList = meshData.vertices;
	std::vector<GLuint>& idxList = meshData.indices;

	int latDiv = div / 2 + 1;

	int vertCount = 2 + (div + 1) * (latDiv - 2);
	int idxCount = div * ((latDiv - 3) * 2 + 2) * 3;

	vertList.reserve(vertCount);
	idxList.reserve(idxCount);

	// add first vert
	vertList.push_back(Vertex(glm::vec3(0, 1, 0), glm::vec3(0, 1, 0), glm::vec3(1, 0, 0), glm::vec2(0, 0)));
	int prevRingStart = 0;
	int curRingStart = 1;
	// add rings
	for (int iLat = 1; iLat < latDiv - 1; ++iLat)
	{
		// (-1, 1)
		float latRatio = (float)iLat / (float)(latDiv - 1);
		float latAngle = (1 - latRatio * 2) * 0.5f * glm::pi<float>();
		float cosLat = glm::cos(latAngle);
		float sinLat = glm::sin(latAngle);
		// add vert on a ring
		for (int iLon = 0; iLon < div + 1; ++iLon)
		{
			float lonRatio = (float)iLon / (float)div;
			float lonAngle = lonRatio * 2 * PI;
			float cosLon = glm::cos(lonAngle);
			float sinLon = glm::sin(lonAngle);
			glm::vec3 pos(cosLon * cosLat, sinLat, sinLon * cosLat);
			glm::vec3 tangent(sinLon, 0, -cosLon);
			vertList.push_back(Vertex(pos, pos, tangent, glm::vec2(lonRatio, latRatio)));
		}
		// add idx on a ring with prev ring
		for (int iLon = 0; iLon < div; ++iLon)
		{
			int next = iLon + 1;
			if (iLat == 1)
			{
				// first ring
				idxList.push_back(prevRingStart);
				idxList.push_back(curRingStart + next);
				idxList.push_back(curRingStart + iLon);
			}
			else
			{
				idxList.push_back(prevRingStart + iLon);
				idxList.push_back(curRingStart + next);
				idxList.push_back(curRingStart + iLon);
				idxList.push_back(prevRingStart + iLon);
				idxList.push_back(prevRingStart + next);
				idxList.push_back(curRingStart + next);
			}
		}
		prevRingStart = curRingStart;
		curRingStart += (div + 1);
	}
	// add last vert
	vertList.push_back(Vertex(glm::vec3(0, -1, 0), glm::vec3(0, -1, 0), glm::vec3(1, 0, 0), glm::vec2(1, 1)));
	// add idx for last ring
	for (int iLon = 0; iLon < div; ++iLon)
	{
		int next = (iLon + 1);
		idxList.push_back(prevRingStart + iLon);
		idxList.push_back(prevRingStart + next);
		idxList.push_back(curRingStart);
	}

	assert(vertList.size() == vertCount);
	assert(idxList.size() == idxCount);
}

glm::vec3 GetTangent(glm::vec3 normal)
{
	glm::vec3 up(0, 1, 0);
	glm::vec3 tangent = glm::cross(up, normal);
	if (glm::dot(tangent, tangent) < KINDA_SMALL_NUMBER)
		return glm::vec3(1, 0, 0);
	return glm::normalize(tangent);
}

void MakeCone(MeshData& meshData, int firstRingVertCount, int level)
{
	std::vector<Vertex>& vertList = meshData.vertices;
	std::vector<GLuint>& idxList = meshData.indices;

	glm::vec3 mainAxis(0, 0, -1);
	glm::vec3 secAxis(0, 1, 0);
	glm::vec3 thrAxis = glm::cross(mainAxis, secAxis);

	static const float normScaler = 1.f / sqrt(2.f);

	// first vert = level 0
	vertList.push_back(Vertex(glm::vec3(0, 0, 0), -mainAxis, thrAxis, glm::vec2(0, 0)));
	int prevRingStart = 0;
	int curRingStart = 1;

	int ringVertCount = firstRingVertCount;
	for (int levelIdx = 1; levelIdx <= level; ++levelIdx)
	{
		float mainAxisValue = (float)levelIdx / (float)(level);
		// vert
		for (int ringIdx = 0; ringIdx <= ringVertCount; ++ringIdx)
		{
			float ratio = (float)ringIdx / (float)ringVertCount;
			float angle = ratio * 2 * PI;
			float cosA = glm::cos(angle);
			float sinA = glm::sin(angle);
			glm::vec3 pos = (mainAxis + secAxis * cosA + thrAxis * sinA) * mainAxisValue;
			glm::vec3 normal = (-mainAxis + secAxis * cosA + thrAxis * sinA) * normScaler;
			glm::vec3 tangent = secAxis * sinA + thrAxis * -cosA;

			vertList.push_back(Vertex(pos, normal, tangent, glm::vec2(ratio, mainAxisValue)));
		}

		// idx
		if (levelIdx == 1)
		{
			for (int ringIdx = 1; ringIdx <= ringVertCount; ++ringIdx)
			{
				idxList.push_back(prevRingStart);
				idxList.push_back(curRingStart + ringIdx);
				idxList.push_back(curRingStart + ringIdx - 1);
			}
		}
		else
		{
			for (int ringIdx = 2; ringIdx <= ringVertCount; ringIdx += 2)
			{
				int prevRingIdx = ringIdx / 2;
				idxList.push_back(prevRingStart + prevRingIdx - 1);
				idxList.push_back(curRingStart + ringIdx - 1);
				idxList.push_back(curRingStart + ringIdx - 2);
				idxList.push_back(prevRingStart + prevRingIdx - 1);
				idxList.push_back(prevRingStart + prevRingIdx);
				idxList.push_back(curRingStart + ringIdx - 1);
				idxList.push_back(prevRingStart + prevRingIdx);
				idxList.push_back(curRingStart + ringIdx);
				idxList.push_back(curRingStart + ringIdx - 1);
			}
		}

		prevRingStart = curRingStart;
		curRingStart += (ringVertCount + 1);

		if (levelIdx < level)
			ringVertCount *= 2;
	}

	// bottom
	for (int ringIdx = 0; ringIdx <= ringVertCount; ++ringIdx)
	{
		float ratio = (float)ringIdx / (float)ringVertCount;
		float angle = ratio * 2 * PI;
		float cosA = glm::cos(angle);
		float sinA = glm::sin(angle);
		glm::vec3 pos = (mainAxis + secAxis * cosA + thrAxis * sinA);
		glm::vec3 tangent = secAxis * sinA + thrAxis * -cosA;

		vertList.push_back(Vertex(pos, mainAxis, tangent, glm::vec2(ratio, 1)));
	}

	prevRingStart = curRingStart;
	curRingStart += (ringVertCount + 1);

	// final vert
	vertList.push_back(Vertex(mainAxis, mainAxis, thrAxis, glm::vec2(0, 0)));

	// idx
	for (int ringIdx = 1; ringIdx <= ringVertCount; ++ringIdx)
	{
		idxList.push_back(prevRingStart + ringIdx - 1);
		idxList.push_back(prevRingStart + ringIdx);
		idxList.push_back(curRingStart);
	}
}

int FindNewVert(const std::vector<glm::ivec3>& newVertTable, int i0, int i1)
{
	for (int i = 0; i < newVertTable.size(); ++i)
	{
		if ((newVertTable[i].x == i0 && newVertTable[i].y == i1) ||
			(newVertTable[i].x == i1 && newVertTable[i].y == i0))
			return newVertTable[i].z;
	}
	return -1;
}

void MakeIcosahedron(MeshData& meshData, int tesLevel)
{
	std::vector<Vertex>& vertList = meshData.vertices;
	std::vector<GLuint>& idxList = meshData.indices;

	vertList.empty();
	idxList.empty();

	int tesPower = 1 << (2 * tesLevel); // = 4 ^ tes
	vertList.reserve(10 * tesPower + 2); // 12 + 30 + 30 * 4 + ... + 30 * 4 ^ (tes-1) = 12 + 30 * (4 ^ tes - 1) / (4 - 1) = 10 * 4 ^ tes + 2
	idxList.reserve(60 * tesPower);

	static const float t = (1.f + sqrt(5.f)) * 0.5f;
	static const float s = 1.f / sqrt(1.f + t*t);

	static const glm::vec3 originVert[12] =
	{
		glm::vec3(1, t, 0) * s,
		glm::vec3(-1, t, 0) * s,
		glm::vec3(-1, -t, 0) * s,
		glm::vec3(1, -t, 0) * s,
		glm::vec3(t, 0, 1) * s,
		glm::vec3(t, 0, -1) * s,
		glm::vec3(-t, 0, -1) * s,
		glm::vec3(-t, 0, 1) * s,
		glm::vec3(0, 1, t) * s,
		glm::vec3(0, -1, t) * s,
		glm::vec3(0, -1, -t) * s,
		glm::vec3(0, 1, -t) * s,
	};

	static const int originIdx[60]
	{
		0, 8, 4,
		0, 4, 5,
		0, 5, 11,
		0, 11, 1,
		0, 1, 8,
		1, 11, 6,
		1, 6, 7,
		1, 7, 8,
		2, 9, 7,
		2, 7, 6,
		2, 6, 10,
		2, 10, 3,
		2, 3, 9,
		3, 10, 5,
		3, 5, 4,
		3, 4, 9,
		4, 8, 9,
		5, 10, 11,
		6, 11, 10,
		7, 9, 8,
	};

	for (int i = 0; i < 12; ++i)
	{
		vertList.push_back(Vertex(originVert[i], originVert[i], GetTangent(originVert[i]), glm::vec2(0, 0)));
	}

	for (int i = 0; i < 60; ++i)
		idxList.push_back(originIdx[i]);

	for (int tesIdx = 0; tesIdx < tesLevel; ++tesIdx)
	{
		int idxCount = idxList.size();
		int newVertStartIdx = vertList.size();

		int newVertCount = idxCount / 2;
		std::vector<glm::ivec3> newVertTable;
		newVertTable.reserve(newVertCount);

		for (int faceIdx = 0; faceIdx < idxCount; faceIdx += 3)
		{
			int i0 = idxList[faceIdx + 0];
			int i1 = idxList[faceIdx + 1];
			int i2 = idxList[faceIdx + 2];
			glm::vec3 v0 = vertList[i0].position;
			glm::vec3 v1 = vertList[i1].position;
			glm::vec3 v2 = vertList[i2].position;
			// new idx
			int i3 = FindNewVert(newVertTable, i0, i1);
			int i4 = FindNewVert(newVertTable, i1, i2);
			int i5 = FindNewVert(newVertTable, i2, i0);
			// new vert
			if (i3 < 0)
			{
				glm::vec3 v3 = glm::normalize((v0 + v1) * 0.5f);
				vertList.push_back(Vertex(v3, v3, GetTangent(v3), glm::vec2(0, 0)));
				i3 = newVertStartIdx;
				++newVertStartIdx;
				newVertTable.push_back(glm::ivec3(i0, i1, i3));
			}
			if (i4 < 0)
			{
				glm::vec3 v4 = glm::normalize((v1 + v2) * 0.5f);
				vertList.push_back(Vertex(v4, v4, GetTangent(v4), glm::vec2(0, 0)));
				i4 = newVertStartIdx;
				++newVertStartIdx;
				newVertTable.push_back(glm::ivec3(i1, i2, i4));
			}
			if (i5 < 0)
			{
				glm::vec3 v5 = glm::normalize((v2 + v0) * 0.5f);
				vertList.push_back(Vertex(v5, v5, GetTangent(v5), glm::vec2(0, 0)));
				i5 = newVertStartIdx;
				++newVertStartIdx;
				newVertTable.push_back(glm::ivec3(i2, i0, i5));
			}

			// change origin idx
			idxList[faceIdx + 0] = i0;
			idxList[faceIdx + 1] = i3;
			idxList[faceIdx + 2] = i5;
			// add idx
			idxList.push_back(i3);
			idxList.push_back(i1);
			idxList.push_back(i4);
			idxList.push_back(i3);
			idxList.push_back(i4);
			idxList.push_back(i5);
			idxList.push_back(i5);
			idxList.push_back(i4);
			idxList.push_back(i2);
		}
	}

}

void MakeQuad(MeshData& meshData)
{
	std::vector<Vertex>& vertList = meshData.vertices;
	std::vector<GLuint>& idxList = meshData.indices;

	vertList.reserve(4);
	idxList.reserve(6);

	vertList.push_back(Vertex(glm::vec3(-1, 1, 0), glm::vec3(0, 0, -1), glm::vec3(1, 0, 0), glm::vec2(0, 1)));
	vertList.push_back(Vertex(glm::vec3(-1, -1, 0), glm::vec3(0, 0, -1), glm::vec3(1, 0, 0), glm::vec2(0, 0)));
	vertList.push_back(Vertex(glm::vec3(1, -1, 0), glm::vec3(0, 0, -1), glm::vec3(1, 0, 0), glm::vec2(1, 0)));
	vertList.push_back(Vertex(glm::vec3(1, 1, 0), glm::vec3(0, 0, -1), glm::vec3(1, 0, 0), glm::vec2(1, 1)));

	idxList.push_back(0);
	idxList.push_back(1);
	idxList.push_back(2);
	idxList.push_back(0);
	idxList.push_back(2);
	idxList.push_back(3);
}
