#pragma once


#include <string>

#include "gl/glew.h"


#include "Containers/Containers.h"
#include "Math/REMath.h"

#include "SDL.h"
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

struct TimestampPair
{
	GLuint m[2];
};

class ScopedProfileTimerCPU
{
	Uint64 start;
	size_t nameSize;

public:
	static RESortedMap<std::string, double> timerMap;
	static REMap<std::string, double> timeStampMap[2];
	static int MapWriteIdx;
	static char fullName[1024];

	static void Swap()
	{
		int MapReadIdx = 1 - MapWriteIdx;
		MapWriteIdx = MapReadIdx;
		for (auto it = timeStampMap[MapReadIdx].begin(); it != timeStampMap[MapReadIdx].end(); ++it)
		{
			// update
			timerMap[it->first] = it->second;
			// clear
			it->second = 0;
		}
	}

	ScopedProfileTimerCPU(const char* inName)
	{
		nameSize = strlen(inName);
		// add to prefix
		strcat_s(fullName, "/");
		strcat_s(fullName, inName);

		start = SDL_GetPerformanceCounter();
	}

	~ScopedProfileTimerCPU()
	{
		Uint64 end = SDL_GetPerformanceCounter();
		double deltaTime = (double)((end - start) * 1000) * gInvPerformanceFreq;
		auto it = timeStampMap[MapWriteIdx].find(fullName);
		if (it != timeStampMap[MapWriteIdx].end())
		{
			// find it
			it->second += deltaTime;
		}
		else
		{
			timeStampMap[MapWriteIdx][fullName] = deltaTime;
		}
		// remove from full name
		size_t len = strlen(fullName);
		fullName[len - nameSize - 1] = 0;
	}
};

class ScopedProfileTimerGPU
{
	TimestampPair query;
	size_t nameSize;

public:
	static RESortedMap<std::string, double> timerMap;
	static REMap<std::string, REArray<TimestampPair>> timeStampPairMap[2];
	static int MapWriteIdx;
	static char fullName[1024];

	static void Swap()
	{
		int MapReadIdx = 1 - MapWriteIdx;
		MapWriteIdx = MapReadIdx;
		for (auto &it = timeStampPairMap[MapReadIdx].begin(); it != timeStampPairMap[MapReadIdx].end(); ++it)
		{
			double elapsed = 0;
			for (int pairIdx = 0; pairIdx < it->second.size(); ++pairIdx)
			{
				GLuint64 elapsedStart = 0, elapsedEnd = 0;
				glGetQueryObjectui64v(it->second[pairIdx].m[0], GL_QUERY_RESULT, &elapsedStart);
				glGetQueryObjectui64v(it->second[pairIdx].m[1], GL_QUERY_RESULT, &elapsedEnd);
				glDeleteQueries(2, it->second[pairIdx].m);
				elapsed += (double)(elapsedEnd - elapsedStart) / (double)1000000;
			}
			//if (elapsed > 0)
			{
				// update
				if (timerMap[it->first] > 0)
					timerMap[it->first] = Lerp(timerMap[it->first], elapsed, 0.2);
				else
					timerMap[it->first] = elapsed;
			}
			// clear
			it->second.clear();
		}
	}

	ScopedProfileTimerGPU(const char* inName)
	{
		nameSize = strlen(inName);
		// add to prefix
		strcat_s(fullName, "/");
		strcat_s(fullName, inName);

		glGenQueries(2, query.m);
		glQueryCounter(query.m[0], GL_TIMESTAMP);
	}

	~ScopedProfileTimerGPU()
	{
		glQueryCounter(query.m[1], GL_TIMESTAMP);
		timeStampPairMap[MapWriteIdx][fullName].push_back(query);
		//auto it = timerMap.find(fullName);
		//if (it == timerMap.end())
		//{
		//	timerMap[fullName] = 0;
		//}
		// remove from full name
		size_t len = strlen(fullName);
		fullName[len - nameSize - 1] = 0;
	}
};
