#pragma once

#include <unordered_map>

// glew
#include "gl/glew.h"

// opengl
#include "SDL_opengl.h"

#include "ShaderLoader.h"

#include "Engine/Profiler.h"

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
		int len = strlen(buffer);
		sprintf_s(buffer + len, bufferSize - len, "[%d]", index);
		return *this;
	}

	inline ShaderNameBuilder& operator() (const char* name)
	{
		int len = strlen(buffer);
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

	GLint positionIdx;
	GLint normalIdx;
	GLint tangentIdx;
	GLint texCoordsIdx;

	GLuint nextTexUnit;

	GLchar vertexFilePath[512];
	GLchar fragmentFilePath[512];

	std::unordered_map <std::string, GLint> TexUnitMap;
	std::vector <ValuePair> UniformLocationMap;

	Shader()
	{
		memset(vertexFilePath, 0, _countof(vertexFilePath) * sizeof(GLchar));
		memset(fragmentFilePath, 0, _countof(fragmentFilePath) * sizeof(GLchar));
	}

	Shader(const GLchar* vertexPath, const GLchar* fragmentPath) : Shader()
	{
		Load(vertexPath, fragmentPath);
	}

	void Load(const GLchar* vertexPath, const GLchar* fragmentPath, bool bAssert = true)
	{
		ShaderInfo vShaderInfo;
		ShaderInfo fShaderInfo;

		if (!LoadShader(vShaderInfo, vertexPath))
		{
			printf("Error: fail to read shader vertex: %s", vertexPath);
			if (bAssert)
				assert(0);
			else
				return;
		}
		if (!LoadShader(fShaderInfo, fragmentPath))
		{
			printf("Error: fail to read shader fragmenet: %s", fragmentPath);
			if (bAssert)
				assert(0);
			else
				return;
		}

		strcpy_s(vertexFilePath, vertexPath);
		strcpy_s(fragmentFilePath, fragmentPath);

		const GLchar* vShaderCode = vShaderInfo.shaderCode.c_str();
		const GLchar* fShaderCode = fShaderInfo.shaderCode.c_str();

		// compile shader
		GLuint vertexID, fragmentID;
		GLint success;
		GLchar infoLog[512];

		// vertex shader
		vertexID = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertexID, 1, &vShaderCode, NULL);
		glCompileShader(vertexID);
		glGetShaderiv(vertexID, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(vertexID, 512, NULL, infoLog);
			printf("Error: vertex shader %s compile fail: %s", vertexPath, infoLog);
			// clean up
			glDeleteShader(vertexID);
			if (bAssert)
				assert(success);
			else
				return;
		}

		// fragment shader
		fragmentID = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragmentID, 1, &fShaderCode, NULL);
		glCompileShader(fragmentID);
		glGetShaderiv(fragmentID, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(fragmentID, 512, NULL, infoLog);
			printf("Error: fragment shader %s compile fail: %s", fragmentPath, infoLog);
			// clean up
			glDeleteShader(vertexID);
			glDeleteShader(fragmentID);
			if (bAssert)
				assert(success);
			else
				return;
		}

		// attach and link
		GLuint newProgramID = glCreateProgram();
		glAttachShader(newProgramID, vertexID);
		glAttachShader(newProgramID, fragmentID);
		glLinkProgram(newProgramID);
		glGetProgramiv(newProgramID, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(newProgramID, 512, NULL, infoLog);
			printf("Error: shader programe link fail: %s", infoLog);
			// clean up
			glDeleteShader(vertexID);
			glDeleteShader(fragmentID);
			glDeleteProgram(newProgramID);
			if (bAssert)
				assert(success);
			else
				return;
		}

		bool bUseDeferredPassTex = false;
		bool bUsePostProcessPassTex = false;
		// process hint
		ShaderInfo* shaderInfoList[2] = { &vShaderInfo, &fShaderInfo };
		for (int shaderInfoIdx = 0; shaderInfoIdx < 2; ++shaderInfoIdx)
		{
			ShaderInfo* shaderInfo = shaderInfoList[shaderInfoIdx];

			bUseDeferredPassTex |= shaderInfo->bUseDeferredPassTex;
			bUsePostProcessPassTex |= shaderInfo->bUsePostProcessPassTex;
		}
		if (bUseDeferredPassTex && bUsePostProcessPassTex)
		{
			glDeleteShader(vertexID);
			glDeleteShader(fragmentID);
			glDeleteProgram(newProgramID);
			if (bAssert)
				assert(false);
			else
				return;
		}
		
		// delete shader
		glDeleteShader(vertexID);
		glDeleteShader(fragmentID);

		// delete old program
		if (programID)
			glDeleteProgram(programID);
		programID = newProgramID;

		// get attribute indices
		positionIdx = GetAttribuleLocation("position");
		normalIdx = GetAttribuleLocation("normal");
		tangentIdx = GetAttribuleLocation("tangent");
		texCoordsIdx = GetAttribuleLocation("texCoords");

		// uniform buffer index
		BindUniformBlock("RenderInfo", RenderInfoBP);
		
		// process uniforms
		nextTexUnit = 0;
		if (bUseDeferredPassTex)
			nextTexUnit = deferredPassTexCount;
		else if (bUsePostProcessPassTex)
			nextTexUnit = postProcessPassTexCount;

		TexUnitMap.clear();
		UniformLocationMap.clear();
		for (int shaderInfoIdx = 0; shaderInfoIdx < 2; ++shaderInfoIdx)
		{
			ShaderInfo* shaderInfo = shaderInfoList[shaderInfoIdx];
			for (auto it = shaderInfo->shaderUniforms.typeMap.begin(); it != shaderInfo->shaderUniforms.typeMap.end(); ++it)
			{
				for (int nameIdx = 0; nameIdx < it->second.size(); ++nameIdx)
				{
					//UniformLocationMap[it->second[nameIdx]] = -1;
					UniformLocationMap.push_back(ValuePair(it->second[nameIdx].c_str(), -1));
					if (it->first.compare("sampler2D") == 0)
					{
						// custom texture unit, init to be -1, don't bind it unless used
						TexUnitMap[it->second[nameIdx]] = -1;
					}
				}
			}
		}
		

		// set texture unit
		Use(); // must use program here
		// deferrd pass
		if (bUseDeferredPassTex)
		{
			SetTextureUnit("gDepthStencilTex", gDepthStencilTexUnit);
			SetTextureUnit("gNormalTex", gNormalTexUnit);
			SetTextureUnit("gAlbedoTex", gAlbedoTexUnit);
			SetTextureUnit("gMaterialTex", gMaterialTexUnit);
		}
		// post process pass
		else if (bUsePostProcessPassTex)
		{
			SetTextureUnit("gDepthStencilTex", gDepthStencilTexUnit);
			SetTextureUnit("gSceneColorTex", gSceneColorTexUnit);
		}
		// custom
		//for (auto it = TexUnitMap.begin(); it != TexUnitMap.end(); ++it)
		//{
		//	SetTextureUnit(it->first.c_str(), it->second);
		//}
	}

	void Use()
	{
		glUseProgram(programID);
	}

	GLint GetAttribuleLocation(const GLchar* name)
	{
		GLint location = -1;
		location = glGetAttribLocation(programID, name);
		if (location == -1)
		{
			printf("%s is not a valid glsl program variable! (vert: %s frag: %s)\n", name, vertexFilePath, fragmentFilePath);
		}
		return location;
	}

	GLint GetUniformLocation_Internal(const GLchar* name)
	{
		GLint location = -1;
		location = glGetUniformLocation(programID, name);
		if (location == -1)
		{
			printf("%s is not a valid glsl program uniform! (vert: %s frag: %s)\n", name, vertexFilePath, fragmentFilePath);
		}
		return location;
	}

	GLint GetUniformLocation(const GLchar* name)
	{
		return GetUniformLocation_Internal(name);
		//auto it = UniformLocationMap.find(name);
		//if (it != UniformLocationMap.end())
		//{
		//	if (it->second < 0)
		//	{
		//		// we haven't store this uniform location yet
		//		it->second = GetUniformLocation_Internal(name);
		//	}
		//	return it->second;
		//}
		//return -1;
		for (int i = 0, ni = (int)UniformLocationMap.size(); i < ni; ++i)
		{
			ValuePair& pair = UniformLocationMap.data()[i];
			if (strcmp(pair.key, name) == 0)
			{
				if(pair.value < 0)
					pair.value = GetUniformLocation_Internal(name);
				return pair.value;
			}
		}
		return - 1;
	}

	void BindUniformBlock(const GLchar* name, GLuint bindingPoint)
	{
		GLuint blockIdx = glGetUniformBlockIndex(programID, name);
		if (blockIdx != GL_INVALID_INDEX)
			glUniformBlockBinding(programID, blockIdx, bindingPoint);
	}

	void SetTextureUnit(const GLchar* name, GLuint texUnit)
	{
		// use internal here, since reserved texture is not in uniform map, and we only get texture location once
		GLint location = GetUniformLocation_Internal(name);
		if (location != -1)
			glUniform1i(location, texUnit);
	}

	GLint GetTextureUnit(const GLchar* name)
	{
		auto it = TexUnitMap.find(name);
		if (it != TexUnitMap.end())
		{
			if (it->second < 0)
			{
				// we haven't bind this texture yet, bind it now
				it->second = nextTexUnit;
				SetTextureUnit(it->first.c_str(), it->second);
				++nextTexUnit;
			}
			return it->second;
		}
		printf("%s is not a valid texture name! (vert: %s frag: %s)\n", name, vertexFilePath, fragmentFilePath);
		return -1;
	}
};
