#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>

#define DUMP_SHADER 0

const int MAX_INCLUDE_DEPTH = 8;

std::map<std::string, std::string> gShaderFileCache;

bool LoadShader(std::string& output, std::string path, int includeDepth = 0)
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
		output.append(it->second);
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

	std::string localOutput;

	const char* token = "#include \"";
	size_t tokenLen = strlen(token);

	char endOfLineChar = '\n';
	size_t posStartOfLine = 0;
	size_t posEndOfLine = content.find(endOfLineChar, posStartOfLine);
	while (posEndOfLine != std::string::npos)
	{
		std::string line = content.substr(posStartOfLine, posEndOfLine - posStartOfLine + 1);
		size_t includeStart = line.find(token);
		if (includeStart != std::string::npos)
		{
			// maybe found an include
			size_t pathStart = includeStart + tokenLen;
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
		else
		{
			// copy the line
			localOutput.append(line);
		}

		posStartOfLine = posEndOfLine + 1;
		posEndOfLine = content.find(endOfLineChar, posStartOfLine);
	}

	// last line
	localOutput.append(content.substr(posStartOfLine));
	localOutput.append("\n");

	// we load the file, now add it to cache
	gShaderFileCache[fileName] = localOutput;
	
	// add to output
	output.append(localOutput);

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
			fputs(output.c_str(), file);
			fclose(file);
		}
	}
#endif
	
	return true;
}