#pragma once


#include "Containers/Containers.h"

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
	TEX,
};

class MaterialParameter
{
public:
	char name[64];
	int offset = 0;
	int count = 0;
	int location = -1; // uniform location for parameters, tex unit for textures
	EMaterialParameterType type;
	bool bDirty = false;

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
		case EMaterialParameterType::TEX:
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
		case EMaterialParameterType::TEX:
			((Texture*)(parameterValues + offset))->Bind(location);
			break;
		}
	}
};

class Material
{
public:

	static REArray<Material*> gMaterialContainer;

	static Material* Create(Shader* inShader)
	{
		Material* mat = new Material(inShader);
		gMaterialContainer.push_back(mat);
		return mat;
	}
	static Material* Create(Material* otherMaterial)
	{
		Material* mat = new Material(otherMaterial);
		gMaterialContainer.push_back(mat);
		return mat;
	}

	Shader* shader;
	REArray<char> parameterData;
	REArray<MaterialParameter> parameterList;

	Material() {}
	Material(Shader* inShader)
	{
		shader = inShader;
		shader->referenceMaterials.insert(this);
	}
	Material(Material* otherMaterial)
	{
		shader = otherMaterial->shader;
		shader->referenceMaterials.insert(this);
		parameterData = otherMaterial->parameterData;
		parameterList = otherMaterial->parameterList;
	}
	~Material()
	{
		if (shader)
			shader->referenceMaterials.erase(this);
	}

	void Reload();
	void Use(struct RenderContext& renderContext);

	void SetParameter(const char* name, char* data, int bytes, EMaterialParameterType type);

	inline void SetParameter(const char* name, Texture* tex)
	{
		SetParameter(name, (char*)tex, sizeof(Texture*), EMaterialParameterType::TEX);
	}
	
	inline void SetParameter(const char* name, const Vector4& value, int count)
	{
		assert(count >= 2 && count <= 4);
		EMaterialParameterType type = (count == 4) ? EMaterialParameterType::VEC4 :
			((count == 3) ? EMaterialParameterType::VEC3 : EMaterialParameterType::VEC2);
		SetParameter(name, (char*)(value.m), count * sizeof(GLfloat), type);
	}
	
	inline void SetParameter(const char* name, GLfloat value)
	{
		SetParameter(name, (char*)(&value), sizeof(GLfloat), EMaterialParameterType::FLOAT);
	}

	inline void SetParameter(const char* name, GLint value)
	{
		SetParameter(name, (char*)(&value), sizeof(GLint), EMaterialParameterType::INT);
	}

	inline void SetParameter(const char* name, const Matrix4& value)
	{
		SetParameter(name, (char*)(value.m), 16 * sizeof(GLfloat), EMaterialParameterType::MAT4);
	}
};