#pragma once

// opengl
#include "SDL_opengl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cassert>

#include "Containers/Containers.h"

class Material;

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
	static const GLuint GlobalLightsRenderInfoBP = 1;

	static const GLuint LocalLightsRenderInfoBP = 0;
	static const GLuint LocalLightsCullingInfoBP = 1;
	static const GLuint LocalLightsShadowMatrixInfoBP = 2;

	// deferred pass
	static const GLuint gDepthStencilTexUnit = 0;
	static const GLuint gNormalTexUnit = 1;
	static const GLuint gAlbedoTexUnit = 2;
	static const GLuint gMaterialTexUnit = 3;
	static const GLuint gCSMTexArrayUnit = 4;
	static const GLuint gShadowTiledTexUnit = 5;
	static const GLuint gShadowCubeTexArrayUnit = 6;

	static const GLuint deferredPassTexCount = 7;

	// post process pass
	//static const GLuint gDepthStencilTexUnit = 0; reuse
	static const GLuint gSceneColorTexUnit = 1;

	static const GLuint postProcessPassTexCount = 2;

	GLuint programID;

	EVertexType vertexType;

	GLuint nextTexUnit;
	GLuint nextImgUnit;

	GLchar vertexFilePath[256];
	GLchar fragmentFilePath[256];
	GLchar geometryFilePath[256];
	GLchar computeFilePath[256];

	RESet<std::string> dependentFileNames;

	RESet<Material*> referenceMaterials;

	REArray<ValuePair> TexUnitList;
	REArray<ValuePair> ImgUnitList;
	REArray<ValuePair> UniformLocationList;

	Shader()
	{
		memset(vertexFilePath, 0, _countof(vertexFilePath) * sizeof(GLchar));
		memset(fragmentFilePath, 0, _countof(fragmentFilePath) * sizeof(GLchar));
		memset(geometryFilePath, 0, _countof(geometryFilePath) * sizeof(GLchar));
		memset(computeFilePath, 0, _countof(computeFilePath) * sizeof(GLchar));
	}

	Shader(const GLchar* vertexPath, const GLchar* fragmentPath) : Shader()
	{
		Load(vertexPath, fragmentPath);
	}

	void Reload()
	{
		if (computeFilePath[0])
		{
			printf("Shader Reload: %s\n", computeFilePath);
			Load(computeFilePath, false);
		}
		else
		{
			printf("Shader Reload: %s %s %s\n", vertexFilePath, geometryFilePath, fragmentFilePath);
			Load(vertexFilePath, geometryFilePath, fragmentFilePath, false);
		}
	}

	void Load(const GLchar* computePath, bool bAssert = true)
	{
		Load(0, 0, 0, computePath, bAssert);
	}
	void Load(const GLchar* vertexPath, const GLchar* fragmentPath, bool bAssert = true)
	{
		Load(vertexPath, 0, fragmentPath, 0, bAssert);
	}
	void Load(const GLchar* vertexPath, const GLchar* geometryPath, const GLchar* fragmentPath, bool bAssert = true)
	{
		Load(vertexPath, geometryPath, fragmentPath, 0, bAssert);
	}
	void Load(const GLchar* vertexPath, const GLchar* geometryPath, const GLchar* fragmentPath, const GLchar* computePath, bool bAssert = true);
	void Use();

	GLint GetAttribuleLocation(const GLchar* name, bool bSilent = false);

	GLint GetUniformLocation_Internal(const GLchar* name, bool bSilent = false);

	GLint GetUniformLocation(const GLchar* name, bool bSilent = false);

	void BindUniformBlock(const GLchar* name, GLuint bindingPoint);
	void BindShaderStorageBlock(const GLchar* name, GLuint bindingPoint);

	// used for both texture and image
	void SetTextureUnit(const GLchar* name, GLuint texUnit, bool bSilent = false);

	GLint GetTextureUnit(const GLchar* name);
	GLint GetImageUnit(const GLchar* name);
};
