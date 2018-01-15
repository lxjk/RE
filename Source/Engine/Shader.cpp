// glew
#include "gl/glew.h"

#include "ShaderLoader.h"

#include "Shader.h"

bool LoadSingleShader(GLenum type, const GLchar* path, GLuint programID, ShaderInfo& outShaderInfo, GLuint& outShaderID, bool bAssert)
{
	if (!LoadShader(outShaderInfo, path))
	{
		printf("Error: fail to read shader file: %s", path);
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
		printf("Error: shader %s compile fail: %s", path, infoLog);
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

void Shader::Load(const GLchar* vertexPath, const GLchar* geometryPath, const GLchar* fragmentPath, const GLchar* computePath, bool bAssert)
{
	GLuint newProgramID = glCreateProgram();

	REArray<ShaderInfo> shaderInfoList;
	REArray<GLuint> shaderIDList;
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
	if (computePath && computePath[0])
	{
		strcpy_s(computeFilePath, computePath);
		shaderInfoList.emplace_back();
		shaderIDList.emplace_back();
		if (!LoadSingleShader(GL_COMPUTE_SHADER, computePath, newProgramID, shaderInfoList[idx], shaderIDList[idx], bAssert))
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
	for (int shaderInfoIdx = 0; shaderInfoIdx < shaderInfoCount; ++shaderInfoIdx)
	{
		ShaderInfo& shaderInfo = shaderInfoList[shaderInfoIdx];

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

	// uniform block index
	BindUniformBlock("RenderInfo", RenderInfoBP);
	BindUniformBlock("GlobalLightsRenderInfo", GlobalLightsRenderInfoBP);

	// shader storage buffer index
	BindShaderStorageBlock("LocalLightsRenderInfo", LocalLightsRenderInfoBP);
	BindShaderStorageBlock("LocalLightsShadowMatrixInfo", LocalLightsShadowMatrixInfoBP);

	// process uniforms
	nextTexUnit = 0;
	if (bUseDeferredPassTex)
		nextTexUnit = deferredPassTexCount;
	else if (bUsePostProcessPassTex)
		nextTexUnit = postProcessPassTexCount;

	nextImgUnit = 0;

	// unregister shader
	for (auto& dependent : dependentFileNames)
	{
		auto it = gShaderFileCache.find(dependent);
		if (it != gShaderFileCache.end())
		{
			it->second.shaders.erase(this);
		}
	}
	dependentFileNames.clear();

	const char* const samplers[] =
	{
		"sampler1D",
		"sampler2D",
		"sampler3D",
		"samplerCube",
		"sampler2DRect",
		"sampler1DArray",
		"sampler2DArray",
		"samplerCubeArray",
		"samplerBuffer",
		"sampler2DMS",
		"sampler2DMSArray"
	};

	const char* const images[] =
	{
		"image1D",
		"image2D",
		"image3D",
		"imageCube",
		"image2DRect",
		"image1DArray",
		"image2DArray",
		"imageCubeArray",
		"imageBuffer",
		"image2DMS",
		"image2DMSArray"
	};

	TexUnitList.clear();
	ImgUnitList.clear();
	UniformLocationList.clear();
	for (int shaderInfoIdx = 0; shaderInfoIdx < shaderInfoCount; ++shaderInfoIdx)
	{
		ShaderInfo& shaderInfo = shaderInfoList[shaderInfoIdx];
		// add dependent
		dependentFileNames.insert(shaderInfo.involvedFiles.begin(), shaderInfo.involvedFiles.end());

		// process uniforms
		for (auto it = shaderInfo.shaderUniforms.typeMap.begin(); it != shaderInfo.shaderUniforms.typeMap.end(); ++it)
		{
			for (int nameIdx = 0; nameIdx < it->second.size(); ++nameIdx)
			{
				UniformLocationList.push_back(ValuePair(it->second[nameIdx].c_str(), -1));
				bool bTexUniform = false;
				for (int i = 0; i < _countof(samplers); ++i)
				{
					if (it->first.compare(samplers[i]) == 0)
					{
						bTexUniform = true;
						break;
					}
				}
				if (bTexUniform)
				{
					// custom texture unit, init to be -1, don't bind it unless used
					TexUnitList.push_back(ValuePair(it->second[nameIdx].c_str(), -1));
					continue;
				}

				bool bImgUniform = false;
				for (int i = 0; i < _countof(images); ++i)
				{
					if (it->first.compare(images[i]) == 0)
					{
						bImgUniform = true;
						break;
					}
				}
				if (bImgUniform)
				{
					// custom texture unit, init to be -1, don't bind it unless used
					ImgUnitList.push_back(ValuePair(it->second[nameIdx].c_str(), -1));
					continue;
				}
			}
		}
	}

	// register shader
	for (auto& dependent : dependentFileNames)
	{
		auto it = gShaderFileCache.find(dependent);
		if (it != gShaderFileCache.end())
		{
			it->second.shaders.insert(this);
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
		SetTextureUnit("gShadowTiledTex", gShadowTiledTexUnit, true);
		SetTextureUnit("gShadowCubeTexArray", gShadowCubeTexArrayUnit, true);
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
		if (computeFilePath[0])
			printf("%s is not a valid glsl program variable! (comp: %s)\n", name, computeFilePath);
		else
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
		if (computeFilePath[0])
			printf("%s is not a valid glsl program uniform! (comp: %s)\n", name, computeFilePath);
		else
			printf("%s is not a valid glsl program uniform! (vert: %s frag: %s)\n", name, vertexFilePath, fragmentFilePath);
	}
	return location;
}

GLint Shader::GetUniformLocation(const GLchar* name, bool bSilent)
{
	//return GetUniformLocation_Internal(name);
	for (int i = 0, ni = (int)UniformLocationList.size(); i < ni; ++i)
	{
		ValuePair& pair = UniformLocationList[i];
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

void Shader::BindShaderStorageBlock(const GLchar* name, GLuint bindingPoint)
{
	GLuint blockIdx = glGetProgramResourceIndex(programID, GL_SHADER_STORAGE_BLOCK, name);
	if (blockIdx != GL_INVALID_INDEX)
		glShaderStorageBlockBinding(programID, blockIdx, bindingPoint);
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
		ValuePair& pair = TexUnitList[i];
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
	if(computeFilePath[0])
		printf("%s is not a valid texture name! (comp: %s)\n", name, computeFilePath);
	else
		printf("%s is not a valid texture name! (vert: %s frag: %s)\n", name, vertexFilePath, fragmentFilePath);
	return -1;
}

GLint Shader::GetImageUnit(const GLchar* name)
{
	for (int i = 0, ni = (int)ImgUnitList.size(); i < ni; ++i)
	{
		ValuePair& pair = ImgUnitList[i];
		if (strcmp(pair.key, name) == 0)
		{
			if (pair.value < 0)
			{
				pair.value = nextImgUnit;
				SetTextureUnit(pair.key, pair.value);
				++nextImgUnit;
			}
			return pair.value;
		}
	}
	if (computeFilePath[0])
		printf("%s is not a valid image name! (comp: %s)\n", name, computeFilePath);
	else
		printf("%s is not a valid image name! (vert: %s frag: %s)\n", name, vertexFilePath, fragmentFilePath);
	return -1;
}