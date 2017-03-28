#pragma once

#include <map>

// glew
#include "gl/glew.h"

// opengl
#include "SDL_opengl.h"

#include "ShaderLoader.h"

class Shader
{
public:

	static const GLuint RenderInfoBP = 0;

	// deferred pass
	static const GLuint gDepthStencilTexUnit = 0;
	static const GLuint gNormalTexUnit = 1;
	static const GLuint gAlbedoTexUnit = 2;
	static const GLuint gMaterialTexUnit = 3;

	static const int deferredPassTexCount = 4;

	// post process pass
	//static const GLuint gDepthStencilTexUnit = 0; reuse
	static const GLuint gSceneColorTexUnit = 1;

	static const int postProcessPassTexCount = 2;

	GLuint programID;

	GLint positionIdx;
	GLint normalIdx;
	GLint tangentIdx;
	GLint texCoordsIdx;

	GLchar vertexFilePath[512];
	GLchar fragmentFilePath[512];

	std::map<std::string, GLuint> TexUnitMap;

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
		int texUnitStart = 0;
		if (bUseDeferredPassTex)
			texUnitStart = deferredPassTexCount;
		else if (bUsePostProcessPassTex)
			texUnitStart = postProcessPassTexCount;

		TexUnitMap.clear();
		for (int shaderInfoIdx = 0; shaderInfoIdx < 2; ++shaderInfoIdx)
		{
			ShaderInfo* shaderInfo = shaderInfoList[shaderInfoIdx];
			for (auto it = shaderInfo->shaderUniforms.typeMap.begin(); it != shaderInfo->shaderUniforms.typeMap.end(); ++it)
			{
				if (it->first.compare("sampler2D") == 0)
				{
					for (int nameIdx = 0; nameIdx < it->second.size(); ++nameIdx)
					{
						TexUnitMap[it->second[nameIdx]] = texUnitStart;
						++texUnitStart;
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
		for (auto it = TexUnitMap.begin(); it != TexUnitMap.end(); ++it)
		{
			SetTextureUnit(it->first.c_str(), it->second);
		}
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

	GLint GetUniformLocation(const GLchar* name)
	{
		GLint location = -1;
		location = glGetUniformLocation(programID, name);
		if (location == -1)
		{
			printf("%s is not a valid glsl program uniform! (vert: %s frag: %s)\n", name, vertexFilePath, fragmentFilePath);
		}
		return location;
	}

	GLint GetUniformLocation(const GLchar* name,  const GLchar* memberName)
	{
		char combinedName[128];
		sprintf_s(combinedName, "%s.%s", name, memberName);
		GLint location = -1;
		location = glGetUniformLocation(programID, combinedName);
		if (location == -1)
		{
			printf("%s is not a valid glsl program uniform! (vert: %s frag: %s)\n", combinedName, vertexFilePath, fragmentFilePath);
		}
		return location;
	}

	GLint GetUniformLocation(const GLchar* name, int index, const GLchar* memberName)
	{
		char combinedName[128];
		sprintf_s(combinedName, "%s[%d].%s", name, index, memberName);
		GLint location = -1;
		location = glGetUniformLocation(programID, combinedName);
		if (location == -1)
		{
			printf("%s is not a valid glsl program uniform! (vert: %s frag: %s)\n", combinedName, vertexFilePath, fragmentFilePath);
		}
		return location;
	}

	void BindUniformBlock(const GLchar* name, GLuint bindingPoint)
	{
		GLuint blockIdx = glGetUniformBlockIndex(programID, name);
		if (blockIdx != GL_INVALID_INDEX)
			glUniformBlockBinding(programID, blockIdx, bindingPoint);
	}

	void SetTextureUnit(const GLchar* name, GLuint texUnit)
	{
		GLint location = GetUniformLocation(name);
		if (location != -1)
			glUniform1i(location, texUnit);
	}

	GLint GetTextureUnit(const GLchar* name)
	{
		auto it = TexUnitMap.find(name);
		if (it != TexUnitMap.end())
		{
			return it->second;
		}
		printf("%s is not a valid texture name! (vert: %s frag: %s)\n", name, vertexFilePath, fragmentFilePath);
		return -1;
	}
};
