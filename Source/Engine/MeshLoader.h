#pragma once

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include "Mesh.h"

void ProcessMesh(std::vector<Mesh*>& output, aiMesh* mesh, const aiScene* scene, std::vector<Material*>& materials)
{
	// add mesh
	int idx = (int)output.size();
	output.push_back(Mesh::Create());
	Mesh* outputMesh = output[idx];

	// create mesh data
	MeshData* meshData = MeshData::Create();
	std::vector<Vertex>& vertList = meshData->vertices;
	std::vector<GLuint>& idxList = meshData->indices;

	bool bHasTexCoord = (mesh->mTextureCoords[0] > 0);
	for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
	{
		glm::vec3 normal(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
		glm::vec3 tangent(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
		glm::vec3 bitangent(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
		float handness = glm::dot(glm::cross(normal, tangent), bitangent) > 0 ? 1.f : -1.f;

		vertList.push_back(
			Vertex(
				glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z),
				normal,
				glm::vec4(tangent, handness),
				bHasTexCoord ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y) : glm::vec2(0, 0))
			);
	}

	for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
	{
		aiFace &face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; ++j)
			idxList.push_back(face.mIndices[j]);
	}

	if (mesh->mMaterialIndex >= 0)
	{
		// find material
		Material* material = materials[mesh->mMaterialIndex];
		outputMesh->Init(meshData, material);
	}
}

void ProcessNode(std::vector<Mesh*>& output, aiNode* node, const aiScene* scene, std::vector<Material*>& materials)
{
	for (unsigned int i = 0; i < node->mNumMeshes; ++i)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		ProcessMesh(output, mesh, scene, materials);
	}
	for (unsigned int i = 0; i < node->mNumChildren; ++i)
	{
		ProcessNode(output, node->mChildren[i], scene, materials);
	}

}
void LoadMesh(std::vector<Mesh*>& output, std::string path, Shader* defaultShader)
{
	Assimp::Importer import;
	const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene ->mRootNode)
	{
		printf("ERROR::ASSIMP::%s\n", import.GetErrorString());
		return;
	}

	std::string localPath;
	size_t lastSeparator = path.rfind('/');
	if (lastSeparator != std::string::npos)
	{
		localPath = path.substr(0, lastSeparator + 1);
	}

	// make materials
	std::vector<Material*> materials;
	for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
	{
		Material* material = Material::Create(defaultShader);
		materials.push_back(material);

		aiMaterial* aiMaterial = scene->mMaterials[i];

		// diffuse
		if (aiMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0)
		{
			aiString str;
			aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &str);
			std::string texPath = localPath;
			texPath.append(str.C_Str());
			Texture2D* tex = Texture2D::FindOrCreate(texPath.c_str(), true, GL_REPEAT, GL_REPEAT);
			material->SetTexture("diffuseTex", tex);
		}

		// normal
		if (aiMaterial->GetTextureCount(aiTextureType_HEIGHT) > 0)
		{
			aiString str;
			aiMaterial->GetTexture(aiTextureType_HEIGHT, 0, &str);
			std::string texPath = localPath;
			texPath.append(str.C_Str());
			Texture2D* tex = Texture2D::FindOrCreate(texPath.c_str(), false, GL_REPEAT, GL_REPEAT);
			material->SetTexture("normalTex", tex);
		}
	}

	ProcessNode(output, scene->mRootNode, scene, materials);
}

