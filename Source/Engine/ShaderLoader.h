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

class ShaderInfo
{
public:
	std::string shaderCode;
	ShaderUniforms shaderUniforms;

	bool bUseDeferredPassTex = false;
	bool bUsePostProcessPassTex = false;

	void Append(const ShaderInfo& other)
	{
		shaderCode.append(other.shaderCode);
		shaderUniforms.Append(other.shaderUniforms);
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

	const char* tokenInclude = "#include \"";
	size_t tokenLenInclude = strlen(tokenInclude);

	const char* tokenUniform = "uniform ";
	size_t tokenLenUniform = strlen(tokenUniform);

	const char* tokenHint = "#hint ";
	size_t tokenLenHint = strlen(tokenHint);

	char endOfLineChar = '\n';
	size_t posStartOfLine = 0;
	size_t posEndOfLine = content.find(endOfLineChar, posStartOfLine);
	while (posEndOfLine != std::string::npos)
	{
		std::string line = content.substr(posStartOfLine, posEndOfLine - posStartOfLine + 1);
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
		// uniform token
		if (bShouldProcessUniform && (tokenStart == std::string::npos))
		{
			tokenStart = line.find(tokenUniform);
			if (tokenStart != std::string::npos)
			{
				size_t typeStart = tokenStart + tokenLenUniform;
				size_t nameEnd = line.find(';');
				if (nameEnd != std::string::npos)
				{
					std::string typeName = line.substr(typeStart, nameEnd - typeStart);
					trim(typeName);
					size_t typeEnd = typeName.find_first_of(" \t");
					size_t nameStart = typeName.find_last_of(" \t");
					if (typeEnd != std::string::npos && nameStart != std::string::npos)
					{
						localOutput.shaderUniforms.typeMap[typeName.substr(0, typeEnd)]
							.push_back(typeName.substr(nameStart + 1, nameEnd - nameStart - 1));
					}
				}
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

	// last line
	localOutput.shaderCode.append(content.substr(posStartOfLine));
	localOutput.shaderCode.append("\n");

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