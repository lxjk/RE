#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <stack>
#include <set>
#include <cassert>

#include "Shader.h"
#include "Util.h"

#define DUMP_SHADER 0

class ShaderUniforms
{
public:
	// <type, nameArray>
	std::unordered_map<std::string, REArray<std::string>> typeMap;

	void Append(const ShaderUniforms& other)
	{
		for (auto itOther = other.typeMap.begin(); itOther != other.typeMap.end(); ++itOther)
		{
			if (itOther->second.size() > 0)
			{
				// insert or find name array according to type
				REArray<std::string>& nameArray = typeMap[itOther->first];
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
	std::unordered_map<std::string, ShaderUniforms> structTypeMap;

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
	std::unordered_map<std::string, std::string> defineMap;

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
	RESet<std::string> involvedFiles;

	EVertexType vertexType = EVertexType::None;

	bool bUseDeferredPassTex = false;
	bool bUsePostProcessPassTex = false;

	bool bNeedReload = true;

	void Append(const ShaderInfo& other)
	{
		shaderCode.append(other.shaderCode);
		shaderUniforms.Append(other.shaderUniforms);
		shaderStructs.Append(other.shaderStructs);
		shaderDefines.Append(other.shaderDefines);
		bUseDeferredPassTex |= other.bUseDeferredPassTex;
		bUsePostProcessPassTex |= other.bUsePostProcessPassTex;
		if (other.vertexType != EVertexType::None)
		{
			assert(vertexType == EVertexType::None);
			vertexType = other.vertexType;
		}
		involvedFiles.insert(other.involvedFiles.begin(), other.involvedFiles.end());
	}

	bool IsValid()
	{
		return shaderCode.size() > 0 && !(bUseDeferredPassTex && bUsePostProcessPassTex);
	}
};

class ShaderFileInfo
{
public:
	std::string fileName;
	ShaderInfo shaderInfo;
	RESet<Shader*> shaders;
	RESet<std::string> childFiles;
};

class ExpressionParser
{
	std::unordered_map<char, int> priority;
	std::stack<int> numStack;
	std::stack<char> opStack;

public:

	ExpressionParser()
	{
		priority['+'] = 1;
		priority['-'] = 1;
		priority['*'] = 2;
		priority['/'] = 2;
	}

	int Calculate(int num1, int num2, char op)
	{
		switch (op)
		{
		case '+': return num1 + num2;
		case '-': return num1 - num2;
		case '*': return num1 * num2;
		case '/': return num1 / num2;
		}
		assert(false);
		return 0;
	}

	int ProcessStack(int inNum, int inPriority)
	{
		// current num
		int num2 = inNum;

		bool bClearTillBracket = (inPriority < 0);

		// look at stack see if we need to to calculation first
		while (opStack.size() > 0)
		{
			const char& topOp = opStack.top();

			// stop at bracket
			if (topOp == '(')
			{
				if (bClearTillBracket)
					opStack.pop();
				break;
			}

			if (bClearTillBracket || inPriority <= priority[topOp])
			{
				// do calculation
				int num1 = numStack.top();
				num2 = Calculate(num1, num2, topOp);
				opStack.pop();
				numStack.pop();
			}
			else
				break;
		}

		return num2;
	}

	int Parse(std::string expression)
	{
		numStack.empty();
		opStack.empty();

		int sign = 1;
		bool bProcessingNum = false;
		int absNum = 0;

		for (int i = 0; i < expression.length(); ++i)
		{
			bool bResetNum = false;

			const char& c = expression[i];
			// digit
			if (c >= '0' && c <= '9')
			{
				absNum = absNum * 10 + (c - '0');
				bProcessingNum = true;
			}
			// negative sign
			else if(c == '-' && !bProcessingNum)
			{
				sign = -sign;
			}
			else if (c == '+' && !bProcessingNum)
			{
				// skip
			}
			// operator
			else if (priority.find(c) != priority.end())
			{
				assert(bProcessingNum);

				// process stack
				int result = ProcessStack(sign * absNum, priority[c]);

				// push num
				numStack.push(result);

				// push op
				opStack.push(c);

				// reset number
				bProcessingNum = false;
				sign = 1;
				absNum = 0;
			}
			else if (c == '(')
			{
				opStack.push(c);
			}
			else if (c == ')')
			{
				// process stack
				int result = ProcessStack(sign * absNum, -1);

				// set processing num
				bProcessingNum = true;
				sign = result >= 0 ? 1 : -1;
				absNum = result >= 0 ? result : -result;
			}
		}

		if (bProcessingNum)
		{
			return ProcessStack(sign * absNum, -1);
		}
		return numStack.top();
	};

};

const int MAX_INCLUDE_DEPTH = 8;

extern std::unordered_map<std::string, ShaderFileInfo> gShaderFileCache;

static bool ParseVariable(ShaderInfo& shaderInfo, ShaderUniforms& outputUniforms, std::string line, size_t offset = 0)
{
	static ExpressionParser expressionParser;

	size_t nameEnd = line.find(';');
	if (nameEnd != std::string::npos)
	{
		std::string typeName = line.substr(offset, nameEnd - offset);
		trim(typeName);
		size_t typeEnd = typeName.find_first_of(" \t");
		size_t nameStart = std::string::npos;
		size_t arrayStartInTypeName = typeName.find('[');
		if (arrayStartInTypeName != std::string::npos)
			nameStart = typeName.find_last_of(" \t", arrayStartInTypeName);
		else
			nameStart = typeName.find_last_of(" \t");
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
				std::unordered_map<std::string, std::string>& defineMap = shaderInfo.shaderDefines.defineMap;
				// replace define
				bool hasDefine = false;
				do
				{
					hasDefine = false;
					for (auto defineIt = defineMap.begin(); defineIt != defineMap.end(); ++defineIt)
					{
						size_t definePos = arrayCountStr.find(defineIt->first);
						if (definePos != std::string::npos)
						{
							hasDefine = true;
							arrayCountStr.replace(definePos, defineIt->first.length(), defineIt->second);
						}
					}
				} while (hasDefine);
				elemCount = expressionParser.Parse(arrayCountStr);
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

static bool LoadShader(ShaderInfo& output, std::string path, int includeDepth = 0)
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

	ShaderFileInfo* shaderFileInfo = 0;
	// do we have it in cache?
	auto it = gShaderFileCache.find(fileName);
	if (it != gShaderFileCache.end())
	{
		// find it
		shaderFileInfo = &it->second;
		// need reload?
		if (it->second.shaderInfo.bNeedReload)
		{
			// reload, un-register child files info
			for (const auto& file : shaderFileInfo->shaderInfo.involvedFiles)
			{
				gShaderFileCache[file].childFiles.erase(fileName);
			}
			// new shader info
			it->second.shaderInfo = ShaderInfo();
		}
		else
		{
			// we are good
			output.Append(it->second.shaderInfo);
			return true;
		}
	}
	else
	{
		// add new info
		auto newPair = gShaderFileCache.emplace(fileName, ShaderFileInfo{});
		shaderFileInfo = &newPair.first->second;
		shaderFileInfo->fileName = fileName;
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

	ShaderInfo& localOutput = shaderFileInfo->shaderInfo;
	localOutput.involvedFiles.insert(fileName);
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
				// texture hint
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
				// vertex type hint
				else if (hint.compare("CommonVertex") == 0)
				{
					localOutput.vertexType = EVertexType::Common;
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
	
	// add to output
	output.Append(localOutput);

	// we are good
	localOutput.bNeedReload = false;

	// register child files
	for (const auto& file : localOutput.involvedFiles)
	{
		if(file != fileName)
			gShaderFileCache[file].childFiles.insert(fileName);
	}

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