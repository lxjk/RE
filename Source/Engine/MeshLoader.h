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
		Vector4_3 normal(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
		Vector4_3 tangent(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
		Vector4_3 bitangent(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
		float handness = (normal.Cross3(tangent)).Dot3(bitangent) > 0 ? 1.f : -1.f;

		vertList.push_back(
			Vertex(
				Vector4_3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z),
				normal,
				Vector4(tangent, handness),
				bHasTexCoord ? Vector4_2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y) : Vector4_2(0, 0))
			);
	}

	for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
	{
		aiFace &face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; ++j)
			idxList.push_back(face.mIndices[j]);
	}

	meshData->CacheCount();

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
			material->SetParameter("hasDiffuseTex", 1);
			material->SetParameter("diffuseTex", tex);
		}
		else
		{
			material->SetParameter("hasDiffuseTex", 0);
		}

		// normal
		if (aiMaterial->GetTextureCount(aiTextureType_HEIGHT) > 0)
		{
			aiString str;
			aiMaterial->GetTexture(aiTextureType_HEIGHT, 0, &str);
			std::string texPath = localPath;
			texPath.append(str.C_Str());
			Texture2D* tex = Texture2D::FindOrCreate(texPath.c_str(), false, GL_REPEAT, GL_REPEAT);
			material->SetParameter("hasNormalTex", 1);
			material->SetParameter("normalTex", tex);
		}
		else
		{
			material->SetParameter("hasNormalTex", 0);
		}

		// rouhness
		if (aiMaterial->GetTextureCount(aiTextureType_SPECULAR) > 0)
		{
			aiString str;
			aiMaterial->GetTexture(aiTextureType_SPECULAR, 0, &str);
			std::string texPath = localPath;
			texPath.append(str.C_Str());
			Texture2D* tex = Texture2D::FindOrCreate(texPath.c_str(), false, GL_REPEAT, GL_REPEAT);
			material->SetParameter("hasRoughnessTex", 1);
			material->SetParameter("roughnessTex", tex);
		}
		else
		{
			material->SetParameter("hasRoughnessTex", 0);
		}
	}

	ProcessNode(output, scene->mRootNode, scene, materials);
}

