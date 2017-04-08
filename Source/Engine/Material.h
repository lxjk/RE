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
	TEX2D,
};

class MaterialParameter
{
public:
	char name[64];
	int offset = 0;
	int count = 0;
	int location = -1; // uniform location for parameters, tex unit for textures
	EMaterialParameterType type;

	inline void SetLocation(Shader* shader)
	{
		switch (type)
		{
		case EMaterialParameterType::INT:
		case EMaterialParameterType::FLOAT:
		case EMaterialParameterType::VEC2:
		case EMaterialParameterType::VEC3:
		case EMaterialParameterType::VEC4:
		case EMaterialParameterType::MAT3:
		case EMaterialParameterType::MAT4:
			location = shader->GetUniformLocation(name);
			break;
		case EMaterialParameterType::TEX2D:
			location = shader->GetTextureUnit(name);
			break;
		}

	}

	inline void SendValue(Shader* shader, char* parameterValues)
	{
		if (location < 0)
		{
			SetLocation(shader);
		}

		if(location < 0)
			return;

		switch(type)
		{
		case EMaterialParameterType::INT:
			glUniform1i(location, *(GLint*)(parameterValues + offset));
			break;
		case EMaterialParameterType::FLOAT:
			glUniform1f(location, *(GLfloat*)(parameterValues + offset));
			break;
		case EMaterialParameterType::VEC2:
			glUniform2fv(location, 1, (GLfloat*)(parameterValues + offset));
			break;
		case EMaterialParameterType::VEC3:
			glUniform3fv(location, 1, (GLfloat*)(parameterValues + offset));
			break;
		case EMaterialParameterType::VEC4:
			glUniform4fv(location, 1, (GLfloat*)(parameterValues + offset));
			break;
		case EMaterialParameterType::MAT3:
			glUniformMatrix3fv(location, 1, GL_FALSE, (GLfloat*)(parameterValues + offset));
			break;
		case EMaterialParameterType::MAT4:
			glUniformMatrix4fv(location, 1, GL_FALSE, (GLfloat*)(parameterValues + offset));
			break;
		case EMaterialParameterType::TEX2D:
			((Texture2D*)(parameterValues + offset))->Bind(location);
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
	std::vector<char> parameterData;
	std::vector<MaterialParameter> parameterList;

	Material() {}
	Material(Shader* inShader)
	{
		shader = inShader;
	}

	void Reload();
	void Use(struct RenderContext& renderContext);

	void SetParameter(const char* name, char* data, int bytes, EMaterialParameterType type);

	inline void SetParameter(const char* name, Texture2D* tex)
	{
		SetParameter(name, (char*)tex, sizeof(Texture2D*), EMaterialParameterType::TEX2D);
	}
	
	inline void SetParameter(const char* name, const glm::vec4& value)
	{
		SetParameter(name, (char*)glm::value_ptr(value), 4 * sizeof(GLfloat), EMaterialParameterType::VEC4);
	}

	inline void SetParameter(const char* name, const glm::vec3& value)
	{
		SetParameter(name, (char*)glm::value_ptr(value), 3 * sizeof(GLfloat), EMaterialParameterType::VEC3);
	}

	inline void SetParameter(const char* name, const glm::vec2& value)
	{
		SetParameter(name, (char*)glm::value_ptr(value), 2 * sizeof(GLfloat), EMaterialParameterType::VEC2);
	}

	inline void SetParameter(const char* name, GLfloat value)
	{
		SetParameter(name, (char*)(&value), sizeof(GLfloat), EMaterialParameterType::FLOAT);
	}

	inline void SetParameter(const char* name, GLint value)
	{
		SetParameter(name, (char*)(&value), sizeof(GLint), EMaterialParameterType::INT);
	}

	inline void SetParameter(const char* name, const glm::mat4& value)
	{
		SetParameter(name, (char*)glm::value_ptr(value), 16 * sizeof(GLfloat), EMaterialParameterType::MAT4);
	}

	inline void SetParameter(const char* name, const glm::mat3& value)
	{
		SetParameter(name, (char*)glm::value_ptr(value), 9 * sizeof(GLfloat), EMaterialParameterType::MAT3);
	}
};