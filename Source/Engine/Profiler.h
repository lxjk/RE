#pragma once

#include <map>
#include <vector>
#include <string>

#include "gl/glew.h"

#include "SDL.h"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "SDL_opengl.h"

#define PROFILE 1
#if PROFILE
#define CPU_SCOPED_PROFILE(name) ScopedProfileTimerCPU _cpu_macro_profiler(name)
#define GPU_SCOPED_PROFILE(name) ScopedProfileTimerGPU _gpu_macro_profiler(name)
#else
#define CPU_SCOPED_PROFILE(name) 
#define GPU_SCOPED_PROFILE(name) 
#endif

extern double gInvPerformanceFreq;

class ScopedProfileTimerCPU
{
	Uint64 start;
	std::string name;

public:
	static std::map<std::string, double> timerMapCPU;
	static std::string fullName;

	ScopedProfileTimerCPU(std::string inName)
	{
		name = inName;
		// add to prefix
		fullName.append("/");
		fullName.append(name);

		start = SDL_GetPerformanceCounter();
	}

	~ScopedProfileTimerCPU()
	{
		Uint64 end = SDL_GetPerformanceCounter();
		double deltaTime = (double)((end - start) * 1000) * gInvPerformanceFreq;
		auto it = timerMapCPU.find(fullName);
		if (it != timerMapCPU.end())
		{
			// find it
			it->second += deltaTime;
		}
		else
		{
			timerMapCPU[fullName] = deltaTime;
		}
		// remove from full name
		fullName = fullName.substr(0, fullName.size() - name.size() - 1);
	}
};
std::map<std::string, double> ScopedProfileTimerCPU::timerMapCPU;
std::string ScopedProfileTimerCPU::fullName;

class ScopedProfileTimerGPU
{
	glm::uvec2 query;
	std::string name;

public:
	static std::map<std::string, double> timerMapGPU;
	static std::map<std::string, std::vector<glm::uvec2>> timeStampPairMapGPU[2];
	static int GPUMapWriteIdx;
	static std::string fullName;

	ScopedProfileTimerGPU(std::string inName)
	{
		name = inName;
		// add to prefix
		fullName.append("/");
		fullName.append(name);

		glGenQueries(2, glm::value_ptr(query));
		glQueryCounter(query[0], GL_TIMESTAMP);
	}

	~ScopedProfileTimerGPU()
	{
		glQueryCounter(query[1], GL_TIMESTAMP);
		timeStampPairMapGPU[GPUMapWriteIdx][fullName].push_back(query);
		auto it = timerMapGPU.find(fullName);
		if (it == timerMapGPU.end())
		{
			timerMapGPU[fullName] = 0;
		}
		// remove from full name
		fullName = fullName.substr(0, fullName.size() - name.size() - 1);
	}
};
std::map<std::string, double> ScopedProfileTimerGPU::timerMapGPU;
std::map<std::string, std::vector<glm::uvec2>> ScopedProfileTimerGPU::timeStampPairMapGPU[2];
int ScopedProfileTimerGPU::GPUMapWriteIdx = 0;
std::string ScopedProfileTimerGPU::fullName;
