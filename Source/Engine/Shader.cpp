// glew
#include "gl/glew.h"

#include "ShaderLoader.h"

#include "Shader.h"

bool LoadSingleShader(GLenum type, const GLchar* path, GLuint programID, ShaderInfo& outShaderInfo, GLuint& outShaderID, bool bAssert)
{
	if (!LoadShader(outShaderInfo, path))
	{
		printf("Error: fail to read shader vertex: %s", path);
		if (bAssert)
			assert(0);
		else
			return false;
	}
	
	const GLchar* shaderCode = outShaderInfo.shaderCode.c_str();

	// compile shader
	GLint success;
	GLchar infoLog[512];

	outShaderID = glCreateShader(type);
	glShaderSource(outShaderID, 1, &shaderCode, NULL);
	glCompileShader(outShaderID);
	glGetShaderiv(outShaderID, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(outShaderID, 512, NULL, infoLog);
		printf("Error: vertex shader %s compile fail: %s", path, infoLog);
		// clean up
		glDeleteShader(outShaderID);
		if (bAssert)
			assert(success);
		else
			return false;
	}

	// attach
	glAttachShader(programID, outShaderID);

	// delete shader
	glDeleteShader(outShaderID);
	return true;
}

void Shader::Load(const GLchar* vertexPath, const GLchar* geometryPath, const GLchar* fragmentPath, bool bAssert)
{
	GLuint newProgramID = glCreateProgram();

	std::vector<ShaderInfo> shaderInfoList;
	std::vector<GLuint> shaderIDList;
	int idx = 0;

	if (vertexPath && vertexPath[0])
	{
		strcpy_s(vertexFilePath, vertexPath);
		shaderInfoList.emplace_back();
		shaderIDList.emplace_back();
		if (!LoadSingleShader(GL_VERTEX_SHADER, vertexPath, newProgramID, shaderInfoList[idx], shaderIDList[idx], bAssert))
		{
			glDeleteProgram(newProgramID);
			return;
		}
		++idx;
	}
	if (fragmentPath && fragmentPath[0])
	{
		strcpy_s(fragmentFilePath, fragmentPath);
		shaderInfoList.emplace_back();
		shaderIDList.emplace_back();
		if (!LoadSingleShader(GL_FRAGMENT_SHADER, fragmentPath, newProgramID, shaderInfoList[idx], shaderIDList[idx], bAssert))
		{
			glDeleteProgram(newProgramID);
			return;
		}
		++idx;
	}
	if (geometryPath && geometryPath[0])
	{
		strcpy_s(geometryFilePath, geometryPath);
		shaderInfoList.emplace_back();
		shaderIDList.emplace_back();
		if (!LoadSingleShader(GL_GEOMETRY_SHADER, geometryPath, newProgramID, shaderInfoList[idx], shaderIDList[idx], bAssert))
		{
			glDeleteProgram(newProgramID);
			return;
		}
		++idx;
	}
		
	// attach and link
	GLint success;
	GLchar infoLog[512];
	glLinkProgram(newProgramID);
	glGetProgramiv(newProgramID, GL_LINK_STATUS, &success);
	// detach everything
	for (int i = 0; i < shaderIDList.size(); ++i)
		glDetachShader(newProgramID, shaderIDList[i]);
	if (!success)
	{
		glGetProgramInfoLog(newProgramID, 512, NULL, infoLog);
		printf("Error: shader programe link fail: %s", infoLog);
		// clean up
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
	int shaderInfoCount = (int)shaderInfoList.size();
	ShaderInfo* shaderInfoListPtr = shaderInfoList.data();
	for (int shaderInfoIdx = 0; shaderInfoIdx < shaderInfoCount; ++shaderInfoIdx)
	{
		ShaderInfo& shaderInfo = shaderInfoListPtr[shaderInfoIdx];

		bUseDeferredPassTex |= shaderInfo.bUseDeferredPassTex;
		bUsePostProcessPassTex |= shaderInfo.bUsePostProcessPassTex;
		if (shaderInfo.vertexType != EVertexType::None)
		{
			assert(vertexType == EVertexType::None);
			vertexType = shaderInfo.vertexType;
		}
	}
	if (bUseDeferredPassTex && bUsePostProcessPassTex)
	{
		glDeleteProgram(newProgramID);
		if (bAssert)
			assert(false);
		else
			return;
	}

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
	for (int shaderInfoIdx = 0; shaderInfoIdx < shaderInfoCount; ++shaderInfoIdx)
	{
		ShaderInfo& shaderInfo = shaderInfoListPtr[shaderInfoIdx];
		for (auto it = shaderInfo.shaderUniforms.typeMap.begin(); it != shaderInfo.shaderUniforms.typeMap.end(); ++it)
		{
			for (int nameIdx = 0; nameIdx < it->second.size(); ++nameIdx)
			{
				UniformLocationList.push_back(ValuePair(it->second[nameIdx].c_str(), -1));
				if (it->first.compare("sampler2D") == 0 || 
					it->first.compare("samplerCube") == 0 ||
					it->first.compare("sampler2DShadow") == 0)
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
		SetTextureUnit("gDepthStencilTex", gDepthStencilTexUnit, true);
		SetTextureUnit("gNormalTex", gNormalTexUnit, true);
		SetTextureUnit("gAlbedoTex", gAlbedoTexUnit, true);
		SetTextureUnit("gMaterialTex", gMaterialTexUnit, true);
	}
	// post process pass
	else if (bUsePostProcessPassTex)
	{
		SetTextureUnit("gDepthStencilTex", gDepthStencilTexUnit, true);
		SetTextureUnit("gSceneColorTex", gSceneColorTexUnit, true);
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

GLint Shader::GetAttribuleLocation(const GLchar* name, bool bSilent)
{
	GLint location = -1;
	location = glGetAttribLocation(programID, name);
	if (location == -1 && !bSilent)
	{
		printf("%s is not a valid glsl program variable! (vert: %s frag: %s)\n", name, vertexFilePath, fragmentFilePath);
	}
	return location;
}

GLint Shader::GetUniformLocation_Internal(const GLchar* name, bool bSilent)
{
	GLint location = -1;
	location = glGetUniformLocation(programID, name);
	if (location == -1 && !bSilent)
	{
		printf("%s is not a valid glsl program uniform! (vert: %s frag: %s)\n", name, vertexFilePath, fragmentFilePath);
	}
	return location;
}

GLint Shader::GetUniformLocation(const GLchar* name, bool bSilent)
{
	//return GetUniformLocation_Internal(name);
	for (int i = 0, ni = (int)UniformLocationList.size(); i < ni; ++i)
	{
		ValuePair& pair = UniformLocationList.data()[i];
		if (strcmp(pair.key, name) == 0)
		{
			if (pair.value < 0)
				pair.value = GetUniformLocation_Internal(name, bSilent);
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

void Shader::SetTextureUnit(const GLchar* name, GLuint texUnit, bool bSilent)
{
	// use internal here, since reserved texture is not in uniform map, and we only get texture location once
	GLint location = GetUniformLocation_Internal(name, bSilent);
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