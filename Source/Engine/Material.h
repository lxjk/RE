#pragma once


#include <vector>
#include <unordered_map>

#include "Shader.h"
#include "Texture2D.h"

enum class EMaterialParameterType
{
	INT,
	FLOAT,
	VEC2,
	VEC3,
	VEC4,
	MAT3,
	MAT4,
};

class MaterialParameter
{
public:
	char name[64];
	int offset = 0;
	int count = 0;
	int uniformLocation = -1;
	EMaterialParameterType type;

	inline void SetUniform(char* parameterValues)
	{
		if (uniformLocation < 0)
			return;

		switch(type)
		{
		case EMaterialParameterType::INT:
			glUniform1i(uniformLocation, *(GLint*)(parameterValues + offset));
			break;
		case EMaterialParameterType::FLOAT:
			glUniform1f(uniformLocation, *(GLfloat*)(parameterValues + offset));
			break;
		case EMaterialParameterType::VEC2:
			glUniform2fv(uniformLocation, 1, (GLfloat*)(parameterValues + offset));
			break;
		case EMaterialParameterType::VEC3:
			glUniform3fv(uniformLocation, 1, (GLfloat*)(parameterValues + offset));
			break;
		case EMaterialParameterType::VEC4:
			glUniform4fv(uniformLocation, 1, (GLfloat*)(parameterValues + offset));
			break;
		case EMaterialParameterType::MAT3:
			glUniformMatrix3fv(uniformLocation, 1, GL_FALSE, (GLfloat*)(parameterValues + offset));
			break;
		case EMaterialParameterType::MAT4:
			glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, (GLfloat*)(parameterValues + offset));
			break;
		}
	}
};

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
	std::unordered_map<const char*, Texture2D*> textureMap;
	std::vector<char> parameterValues;
	std::vector<MaterialParameter> parameterList;

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

		// set parameters
		for (int i = 0, ni = (int)parameterList.size(); i < ni; ++i)
		{
			MaterialParameter& param = parameterList.data()[i];
			if (param.uniformLocation < 0)
				param.uniformLocation = shader->GetUniformLocation(param.name);
			param.SetUniform(parameterValues.data());
		}
	}

	inline void SetTexture(const char* name, Texture2D* tex)
	{
		textureMap[name] = tex;
	}

	inline void SetParameter(const char* name, char* data, int bytes, EMaterialParameterType type)
	{
		MaterialParameter* params = 0;
		int paramListSize = (int)parameterList.size();
		for (int i = 0; i < paramListSize; ++i)
		{
			//if (parameterList.data()[i].name[0] == name[0])
			if (strcmp(parameterList.data()[i].name, name) == 0)
			{
				params = &parameterList.data()[i];
				break;
			}
		}
		if (!params)
		{
			parameterList.push_back(MaterialParameter());
			params = &parameterList[paramListSize];
			strcpy_s(params->name, name);
			params->offset = parameterValues.size();
			params->count = bytes;
			params->type = type;

			// add data
			parameterValues.resize(params->offset + bytes);
			memcpy_s(parameterValues.data() + params->offset, params->count, data, bytes);
		}
		else
		{
			assert(params->count == bytes);
			assert(params->type == type);
			memcpy_s(parameterValues.data() + params->offset, params->count, data, bytes);
		}
	}
	
	inline void SetParameter(const char* name, const glm::vec4& value, bool immediate = false)
	{
		if (immediate)
			glUniform4fv(shader->GetUniformLocation(name), 1, glm::value_ptr(value));
		else
			SetParameter(name, (char*)glm::value_ptr(value), 4 * sizeof(GLfloat), EMaterialParameterType::VEC4);
	}

	inline void SetParameter(const char* name, const glm::vec3& value, bool immediate = false)
	{
		if (immediate)
			glUniform3fv(shader->GetUniformLocation(name), 1, glm::value_ptr(value));
		else
			SetParameter(name, (char*)glm::value_ptr(value), 3 * sizeof(GLfloat), EMaterialParameterType::VEC3);
	}

	inline void SetParameter(const char* name, const glm::vec2& value, bool immediate = false)
	{
		if (immediate)
			glUniform2fv(shader->GetUniformLocation(name), 1, glm::value_ptr(value));
		else
			SetParameter(name, (char*)glm::value_ptr(value), 2 * sizeof(GLfloat), EMaterialParameterType::VEC2);
	}

	inline void SetParameter(const char* name, GLfloat value, bool immediate = false)
	{
		if (immediate)
			glUniform1f(shader->GetUniformLocation(name), value);
		else
			SetParameter(name, (char*)(&value), sizeof(GLfloat), EMaterialParameterType::FLOAT);
	}

	inline void SetParameter(const char* name, GLint value, bool immediate = false)
	{
		if (immediate)
			glUniform1i(shader->GetUniformLocation(name), value);
		else
			SetParameter(name, (char*)(&value), sizeof(GLint), EMaterialParameterType::INT);
	}

	inline void SetParameter(const char* name, const glm::mat4& value, bool immediate = false)
	{
		if (immediate)
			glUniformMatrix4fv(shader->GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
		else
			SetParameter(name, (char*)glm::value_ptr(value), 16 * sizeof(GLfloat), EMaterialParameterType::MAT4);
	}

	inline void SetParameter(const char* name, const glm::mat3& value, bool immediate = false)
	{
		if (immediate)
			glUniformMatrix3fv(shader->GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
		else
			SetParameter(name, (char*)glm::value_ptr(value), 9 * sizeof(GLfloat), EMaterialParameterType::MAT3);
	}
};

std::vector<Material*> Material::gMaterialContainer;