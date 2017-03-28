#pragma once


#include <vector>
#include <map>

#include "Shader.h"
#include "Texture2D.h"

class Material
{
public:

	static std::vector<Material*> gMaterialContainer;

	static Material* Create(Shader* inShader)
	{
		Material* mat = new Material(inShader);
		gMaterialContainer.push_back(mat);
		return mat;
	}

	Shader* shader;
	std::map<const char*, Texture2D*> textureMap;

	Material() {}
	Material(Shader* inShader)
	{
		shader = inShader;
	}

	void Use()
	{
		if (!shader)
			return;

		// use shader
		shader->Use();

		// bind textures
		for (auto it = textureMap.begin(); it != textureMap.end(); ++it)
		{
			if (!it->second)
				continue;

			GLint texUnit = shader->GetTextureUnit(it->first);
			if (texUnit < 0)
				continue;

			it->second->Bind(texUnit);
		}
	}

	void SetTexture(const char* name, Texture2D* tex)
	{
		textureMap[name] = tex;
	}
};

std::vector<Material*> Material::gMaterialContainer;