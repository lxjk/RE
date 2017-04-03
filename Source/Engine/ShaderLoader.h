#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>

#include "Util.h"

#define DUMP_SHADER 0

class ShaderUniforms
{
public:
	// <type, nameArray>
	std::map<std::string, std::vector<std::string>> typeMap;

	void Append(const ShaderUniforms& other)
	{
		for (auto itOther = other.typeMap.begin(); itOther != other.typeMap.end(); ++itOther)
		{
			if (itOther->second.size() > 0)
			{
				// insert or find name array according to type
				std::vector<std::string>& nameArray = typeMap[itOther->first];
				// append other
				nameArray.insert(nameArray.begin(), itOther->second.begin(), itOther->second.end());
			}
		}
	}
};

class ShaderStructs
{
public:
	// <struct type, content>
	std::map<std::string, ShaderUniforms> structTypeMap;

	void Append(const ShaderStructs& other)
	{
		for (auto itOther = other.structTypeMap.begin(); itOther != other.structTypeMap.end(); ++itOther)
		{
			auto findIt = structTypeMap.find(itOther->first);
			if (findIt == structTypeMap.end())
			{
				// add new struct type
				structTypeMap[itOther->first] = itOther->second;
			}
		}
	}
};

class ShaderDefines
{
public:
	// <define name, value>
	std::map<std::string, std::string> defineMap;

	void Append(const ShaderDefines& other)
	{
		for (auto itOther = other.defineMap.begin(); itOther != other.defineMap.end(); ++itOther)
		{
			auto findIt = defineMap.find(itOther->first);
			if (findIt == defineMap.end())
			{
				// add new define
				defineMap[itOther->first] = itOther->second;
			}
		}
	}
};

class ShaderInfo
{
public:
	std::string shaderCode;
	ShaderUniforms shaderUniforms;
	ShaderStructs shaderStructs;
	ShaderDefines shaderDefines;

	bool bUseDeferredPassTex = false;
	bool bUsePostProcessPassTex = false;

	void Append(const ShaderInfo& other)
	{
		shaderCode.append(other.shaderCode);
		shaderUniforms.Append(other.shaderUniforms);
		shaderStructs.Append(other.shaderStructs);
		shaderDefines.Append(other.shaderDefines);
		bUseDeferredPassTex |= other.bUseDeferredPassTex;
		bUsePostProcessPassTex |= other.bUsePostProcessPassTex;
	}

	bool IsValid()
	{
		return shaderCode.size() > 0 && !(bUseDeferredPassTex && bUsePostProcessPassTex);
	}
};

const int MAX_INCLUDE_DEPTH = 8;

std::map<std::string, ShaderInfo> gShaderFileCache;

bool ParseVariable(ShaderInfo& shaderInfo, ShaderUniforms& outputUniforms, std::string line, size_t offset = 0)
{
	size_t nameEnd = line.find(';');
	if (nameEnd != std::string::npos)
	{
		std::string typeName = line.substr(offset, nameEnd - offset);
		trim(typeName);
		size_t typeEnd = typeName.find_first_of(" \t");
		size_t nameStart = typeName.find_last_of(" \t");
		if (typeEnd != std::string::npos && nameStart != std::string::npos)
		{
			std::string type = typeName.substr(0, typeEnd);
			std::string name = typeName.substr(nameStart + 1, nameEnd - nameStart - 1);
			// is it an array?
			int elemCount = 0;
			size_t arrayStart = name.find('[');
			size_t arrayEnd = name.find(']');
			if (arrayStart != std::string::npos && arrayEnd != std::string::npos && arrayEnd > arrayStart)
			{
				std::string arrayCountStr = name.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
				trim(arrayCountStr);
				name = name.substr(0, arrayStart);
				// check define
				auto defineIt = shaderInfo.shaderDefines.defineMap.find(arrayCountStr);
				while (defineIt != shaderInfo.shaderDefines.defineMap.end())
				{
					arrayCountStr = defineIt->second;
					defineIt = shaderInfo.shaderDefines.defineMap.find(arrayCountStr);
				}
				elemCount = atoi(arrayCountStr.c_str());
			}
			// is the type a known struct?
			auto structIt = shaderInfo.shaderStructs.structTypeMap.find(type);
			if (structIt != shaderInfo.shaderStructs.structTypeMap.end())
			{
				// we are a struct, decode it's variables
				for (auto it = structIt->second.typeMap.begin(); it != structIt->second.typeMap.end(); ++it)
				{
					for (int nameIdx = 0; nameIdx < it->second.size(); ++nameIdx)
					{
						if (elemCount > 0)
						{
							for (int elemIdx = 0; elemIdx < elemCount; ++elemIdx)
							{
								char tmp[128];
								sprintf_s(tmp, "%s[%d].%s", name.c_str(), elemIdx, it->second[nameIdx].c_str());
								outputUniforms.typeMap[it->first].push_back(tmp);
							}
						}
						else
						{
							char tmp[128];
							sprintf_s(tmp, "%s.%s", name.c_str(), it->second[nameIdx].c_str());
							outputUniforms.typeMap[it->first].push_back(tmp);
						}
					}
				}
			}
			else
			{
				if (elemCount > 0)
				{
					for (int elemIdx = 0; elemIdx < elemCount; ++elemIdx)
					{
						char tmp[128];
						sprintf_s(tmp, "%s[%d]", name.c_str(), elemIdx);
						outputUniforms.typeMap[type].push_back(tmp);
					}
				}
				else
					outputUniforms.typeMap[type].push_back(name);
			}
			return true;
		}
	}
	return false;
}

bool LoadShader(ShaderInfo& output, std::string path, int includeDepth = 0)
{
	if (includeDepth > MAX_INCLUDE_DEPTH)
	{
		printf("exceed max include depth %d, file: %s\n", includeDepth, path.c_str());
		return false;
	}
	
	// parse path
	std::string localPath;
	std::string fileName;
	size_t lastSeparator = path.rfind('/');
	if (lastSeparator != std::string::npos)
	{
		localPath = path.substr(0, lastSeparator + 1);
		fileName = path.substr(lastSeparator + 1);
	}
	else
	{
		fileName = path;
	}

	// do we have it in cache?
	auto it = gShaderFileCache.find(fileName);
	if (it != gShaderFileCache.end())
	{
		// find it
		output.Append(it->second);
		return true;
	}

	// we need to load from disk
	std::string content;
	std::ifstream shaderFile;
	shaderFile.exceptions(std::ifstream::badbit);

	try
	{
		// open files
		shaderFile.open(path);
		std::stringstream shaderStream;
		shaderStream << shaderFile.rdbuf();
		shaderFile.close();
		content = shaderStream.str();
	}
	catch (std::ifstream::failure e)
	{
		printf("Error: fail to read file: %s\n", path.c_str());
		return false;
	}

	if (content.size() == 0)
	{
		printf("Error: empty file or fail to read file: %s\n", path.c_str());
		return false;
	}

	ShaderInfo localOutput;
	bool bShouldProcessUniform = true;
	bool bInsideStruct = false;
	ShaderUniforms* activeStructUniforms = 0;

	const char* tokenInclude = "#include \"";
	size_t tokenLenInclude = strlen(tokenInclude);

	const char* tokenHint = "#hint ";
	size_t tokenLenHint = strlen(tokenHint);

	const char* tokenDefine = "#define ";
	size_t tokenLenDefine = strlen(tokenDefine);

	const char* tokenUniform = "uniform ";
	size_t tokenLenUniform = strlen(tokenUniform);

	const char* tokenStruct = "struct ";
	size_t tokenLenStruct = strlen(tokenStruct);

	char endOfLineChar = '\n';
	// add end of line
	content.append("\n");
	size_t posStartOfLine = 0;
	size_t posEndOfLine = content.find(endOfLineChar, posStartOfLine);
	while (posEndOfLine != std::string::npos)
	{
		std::string line = content.substr(posStartOfLine, posEndOfLine - posStartOfLine + 1);
		// only trim start
		trim(line, true, false);
		size_t tokenStart = std::string::npos;
		bool bShouldCopy = true;
		// include token
		if (tokenStart == std::string::npos)
		{
			tokenStart = line.find(tokenInclude);
			if (tokenStart != std::string::npos)
			{
				bShouldCopy = false;
				// maybe found an include
				size_t pathStart = tokenStart + tokenLenInclude;
				// do we have an end?
				size_t pathEnd = line.find('\"', pathStart);
				if (pathEnd != std::string::npos && pathEnd - pathStart > 1)
				{
					// we have a valid end
					std::string includePath = localPath;
					includePath.append(line.substr(pathStart, pathEnd - pathStart));
					if (!LoadShader(localOutput, includePath, includeDepth + 1))
					{
						return false;
					}
				}
			}
		}
		// hint token
		if (tokenStart == std::string::npos)
		{
			tokenStart = line.find(tokenHint);
			if (tokenStart != std::string::npos)
			{
				bShouldCopy = false;
				// maybe found an hint
				size_t hintStart = tokenStart + tokenLenHint;
				std::string hint = line.substr(hintStart);
				trim(hint);
				if (hint.compare("UseDeferredPassTex") == 0)
				{
					localOutput.bUseDeferredPassTex = true;
					bShouldProcessUniform = false;
				}
				else if (hint.compare("UsePostProcessPassTex") == 0)
				{
					localOutput.bUsePostProcessPassTex = true;
					bShouldProcessUniform = false;
				}
			}
		}
		// define token
		if (tokenStart == std::string::npos)
		{
			tokenStart = line.find(tokenDefine);
			if (tokenStart != std::string::npos)
			{
				// maybe found an define
				size_t defineStart = tokenStart + tokenLenDefine;
				size_t valueEnd = line.find('\n');
				if (valueEnd != std::string::npos)
				{
					std::string defineValue = line.substr(defineStart, valueEnd - defineStart);
					trim(defineValue);
					size_t defineEnd = defineValue.find_first_of(" \t");
					size_t valueStart = defineValue.find_last_of(" \t");
					if (defineEnd != std::string::npos && valueStart != std::string::npos)
					{
						std::string define = defineValue.substr(0, defineEnd);
						std::string value = defineValue.substr(valueStart + 1, valueEnd - valueStart - 1);
						localOutput.shaderDefines.defineMap[define] = value;
					}
				}
			}
		}
		// uniform token
		if (bShouldProcessUniform && (tokenStart == std::string::npos))
		{
			tokenStart = line.find(tokenUniform);
			if (tokenStart != std::string::npos)
			{
				size_t typeStart = tokenStart + tokenLenUniform;
				ParseVariable(localOutput, localOutput.shaderUniforms, line, typeStart);
			}
		}
		// struct token
		if (tokenStart == std::string::npos)
		{
			tokenStart = line.find(tokenStruct);
			if (tokenStart != std::string::npos)
			{
				// maybe found a struct
				size_t structStart = tokenStart + tokenLenStruct;
				size_t structEnd = line.find_first_of(" {\t\n", structStart);
				if (structEnd != std::string::npos)
				{
					std::string structType = line.substr(structStart, structEnd - structStart);
					trim(structType);
					bInsideStruct = true;
					activeStructUniforms = &localOutput.shaderStructs.structTypeMap[structType];
				}
			}
		}
		// inside struct
		if (bInsideStruct)
		{
			if (line.find('}') != std::string::npos)
			{
				bInsideStruct = false;
				activeStructUniforms = 0;
			}
			else if(activeStructUniforms)
			{
				ParseVariable(localOutput, *activeStructUniforms, line);
			}
		}
		// normal line
		if (bShouldCopy)
		{
			// copy the line
			localOutput.shaderCode.append(line);
		}

		posStartOfLine = posEndOfLine + 1;
		posEndOfLine = content.find(endOfLineChar, posStartOfLine);
	}

	//// last line
	//localOutput.shaderCode.append(content.substr(posStartOfLine));
	//localOutput.shaderCode.append("\n");

	assert(localOutput.IsValid());

	// we load the file, now add it to cache
	gShaderFileCache[fileName] = localOutput;
	
	// add to output
	output.Append(localOutput);

#if DUMP_SHADER
	// only dump top level
	if (includeDepth == 0)
	{
		FILE *file;
		char dumpPath[128];
		sprintf_s(dumpPath, "ShaderDebug/%s", fileName.c_str());
		fopen_s(&file, dumpPath, "w");
		if (file)
		{
			fputs(output.shaderCode.c_str(), file);
			fclose(file);
		}
	}
#endif
	
	return true;
}