// glew
#include "gl/glew.h"

#include "ShaderLoader.h"

#include "Shader.h"

void Shader::Load(const GLchar* vertexPath, const GLchar* fragmentPath, bool bAssert)
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
	vertexType = EVertexType::None;
	// process hint
	ShaderInfo* shaderInfoList[2] = { &vShaderInfo, &fShaderInfo };
	for (int shaderInfoIdx = 0; shaderInfoIdx < 2; ++shaderInfoIdx)
	{
		ShaderInfo* shaderInfo = shaderInfoList[shaderInfoIdx];

		bUseDeferredPassTex |= shaderInfo->bUseDeferredPassTex;
		bUsePostProcessPassTex |= shaderInfo->bUsePostProcessPassTex;
		if (shaderInfo->vertexType != EVertexType::None)
		{
			assert(vertexType == EVertexType::None);
			vertexType = shaderInfo->vertexType;
		}
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

	// uniform buffer index
	BindUniformBlock("RenderInfo", RenderInfoBP);

	// process uniforms
	nextTexUnit = 0;
	if (bUseDeferredPassTex)
		nextTexUnit = deferredPassTexCount;
	else if (bUsePostProcessPassTex)
		nextTexUnit = postProcessPassTexCount;

	TexUnitList.clear();
	UniformLocationList.clear();
	for (int shaderInfoIdx = 0; shaderInfoIdx < 2; ++shaderInfoIdx)
	{
		ShaderInfo* shaderInfo = shaderInfoList[shaderInfoIdx];
		for (auto it = shaderInfo->shaderUniforms.typeMap.begin(); it != shaderInfo->shaderUniforms.typeMap.end(); ++it)
		{
			for (int nameIdx = 0; nameIdx < it->second.size(); ++nameIdx)
			{
				UniformLocationList.push_back(ValuePair(it->second[nameIdx].c_str(), -1));
				if (it->first.compare("sampler2D") == 0)
				{
					// custom texture unit, init to be -1, don't bind it unless used
					TexUnitList.push_back(ValuePair(it->second[nameIdx].c_str(), -1));
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
	//for (auto it = TexUnitList.begin(); it != TexUnitList.end(); ++it)
	//{
	//	SetTextureUnit(it->first.c_str(), it->second);
	//}
}

void Shader::Use()
{
	glUseProgram(programID);
}

GLint Shader::GetAttribuleLocation(const GLchar* name)
{
	GLint location = -1;
	location = glGetAttribLocation(programID, name);
	if (location == -1)
	{
		printf("%s is not a valid glsl program variable! (vert: %s frag: %s)\n", name, vertexFilePath, fragmentFilePath);
	}
	return location;
}

GLint Shader::GetUniformLocation_Internal(const GLchar* name)
{
	GLint location = -1;
	location = glGetUniformLocation(programID, name);
	if (location == -1)
	{
		printf("%s is not a valid glsl program uniform! (vert: %s frag: %s)\n", name, vertexFilePath, fragmentFilePath);
	}
	return location;
}

GLint Shader::GetUniformLocation(const GLchar* name)
{
	//return GetUniformLocation_Internal(name);
	for (int i = 0, ni = (int)UniformLocationList.size(); i < ni; ++i)
	{
		ValuePair& pair = UniformLocationList.data()[i];
		if (strcmp(pair.key, name) == 0)
		{
			if (pair.value < 0)
				pair.value = GetUniformLocation_Internal(name);
			return pair.value;
		}
	}
	return -1;
}

void Shader::BindUniformBlock(const GLchar* name, GLuint bindingPoint)
{
	GLuint blockIdx = glGetUniformBlockIndex(programID, name);
	if (blockIdx != GL_INVALID_INDEX)
		glUniformBlockBinding(programID, blockIdx, bindingPoint);
}

void Shader::SetTextureUnit(const GLchar* name, GLuint texUnit)
{
	// use internal here, since reserved texture is not in uniform map, and we only get texture location once
	GLint location = GetUniformLocation_Internal(name);
	if (location != -1)
		glUniform1i(location, texUnit);
}

GLint Shader::GetTextureUnit(const GLchar* name)
{
	for (int i = 0, ni = (int)TexUnitList.size(); i < ni; ++i)
	{
		ValuePair& pair = TexUnitList.data()[i];
		if (strcmp(pair.key, name) == 0)
		{
			if (pair.value < 0)
			{
				pair.value = nextTexUnit;
				SetTextureUnit(pair.key, pair.value);
				++nextTexUnit;
			}
			return pair.value;
		}
	}
	printf("%s is not a valid texture name! (vert: %s frag: %s)\n", name, vertexFilePath, fragmentFilePath);
	return -1;
}