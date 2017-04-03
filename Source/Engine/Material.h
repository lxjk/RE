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

	inline void SetTexture(const char* name, Texture2D* tex)
	{
		textureMap[name] = tex;
	}

	inline void SetParameter(const char* name, const glm::vec4& value)
	{
		glUniform4fv(shader->GetUniformLocation(name), 1, glm::value_ptr(value));
	}

	inline void SetParameter(const char* name, const glm::vec3& value)
	{
		glUniform3fv(shader->GetUniformLocation(name), 1, glm::value_ptr(value));
	}

	inline void SetParameter(const char* name, const glm::vec2& value)
	{
		glUniform2fv(shader->GetUniformLocation(name), 1, glm::value_ptr(value));
	}

	inline void SetParameter(const char* name, float value)
	{
		glUniform1f(shader->GetUniformLocation(name), value);
	}

	inline void SetParameter(const char* name, int value)
	{
		glUniform1i(shader->GetUniformLocation(name), value);
	}

	inline void SetParameter(const char* name, const glm::mat4& value)
	{
		glUniformMatrix4fv(shader->GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
	}

	inline void SetParameter(const char* name, const glm::mat3& value)
	{
		glUniformMatrix3fv(shader->GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
	}
};

std::vector<Material*> Material::gMaterialContainer;