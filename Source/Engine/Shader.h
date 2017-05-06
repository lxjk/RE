#pragma once

// opengl
#include "SDL_opengl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <cassert>

enum class EVertexType
{
	None,
	Common,
};

class ShaderNameBuilder
{
public:
	static const size_t bufferSize = 128;
	char buffer[bufferSize];

	ShaderNameBuilder(const char* inName)
	{
		strcpy_s(buffer, inName);
	}

	inline ShaderNameBuilder& operator[] (int index)
	{
		int len = (int)strlen(buffer);
		sprintf_s(buffer + len, bufferSize - len, "[%d]", index);
		return *this;
	}

	inline ShaderNameBuilder& operator() (const char* name)
	{
		int len = (int)strlen(buffer);
		sprintf_s(buffer + len, bufferSize - len, ".%s", name);
		return *this;
	}

	inline const char* c_str()
	{
		return buffer;
	}
};

struct ValuePair
{
	char key[128];
	GLint value;

	ValuePair(const char* inKey, GLint inValue) :
		value(inValue)
	{
		strcpy_s(key, inKey);
	}
};

class Shader
{
public:

	static const GLuint RenderInfoBP = 0;

	// deferred pass
	static const GLuint gDepthStencilTexUnit = 0;
	static const GLuint gNormalTexUnit = 1;
	static const GLuint gAlbedoTexUnit = 2;
	static const GLuint gMaterialTexUnit = 3;

	static const GLuint deferredPassTexCount = 4;

	// post process pass
	//static const GLuint gDepthStencilTexUnit = 0; reuse
	static const GLuint gSceneColorTexUnit = 1;

	static const GLuint postProcessPassTexCount = 2;

	GLuint programID;

	EVertexType vertexType;

	GLuint nextTexUnit;

	GLchar vertexFilePath[256];
	GLchar fragmentFilePath[256];
	GLchar geometryFilePath[256];

	std::vector<ValuePair> TexUnitList;
	std::vector<ValuePair> UniformLocationList;

	Shader()
	{
		memset(vertexFilePath, 0, _countof(vertexFilePath) * sizeof(GLchar));
		memset(fragmentFilePath, 0, _countof(fragmentFilePath) * sizeof(GLchar));
		memset(geometryFilePath, 0, _countof(geometryFilePath) * sizeof(GLchar));
	}

	Shader(const GLchar* vertexPath, const GLchar* fragmentPath) : Shader()
	{
		Load(vertexPath, fragmentPath);
	}

	void Reload()
	{
		Load(vertexFilePath, geometryFilePath, fragmentFilePath, false);
	}

	void Load(const GLchar* vertexPath, const GLchar* fragmentPath, bool bAssert = true)
	{
		Load(vertexPath, 0, fragmentPath, bAssert);
	}
	void Load(const GLchar* vertexPath, const GLchar* geometryPath, const GLchar* fragmentPath, bool bAssert = true);
	void Use();

	GLint GetAttribuleLocation(const GLchar* name);

	GLint GetUniformLocation_Internal(const GLchar* name);

	GLint GetUniformLocation(const GLchar* name);

	void BindUniformBlock(const GLchar* name, GLuint bindingPoint);

	void SetTextureUnit(const GLchar* name, GLuint texUnit);

	GLint GetTextureUnit(const GLchar* name);
};
