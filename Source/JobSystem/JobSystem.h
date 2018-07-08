#pragma once

#include <thread>
#include <atomic>

#include "Containers/Containers.h"

#define USE_JOB_SYSTEM 1

typedef void(*JobEntryPoint)(void* jobParamPtr);

#define JOB_ENTRY_POINT(funcName) void funcName(void* jobParamPtr)
#define JOB_METHOD_ENTRY_POINT(funcName) static void funcName(void* jobParamPtr)

#define BEGIN_JOB_INLINE(structName, funcName) struct structName { JOB_METHOD_ENTRY_POINT(funcName) {

#define BEGIN_JOB_INLINE_ONE_PARAM(structName, funcName, type0, name0, type1, name1) struct structName {\
	type0 name0;\
	JOB_METHOD_ENTRY_POINT(funcName) {\
		structName * structPtr = ( structName * ) jobParamPtr;\
		type0 & name0 = structPtr->name0;

#define BEGIN_JOB_INLINE_TWO_PARAMS(structName, funcName, type0, name0, type1, name1) struct structName {\
	type0 name0;\
	type1 name1;\
	JOB_METHOD_ENTRY_POINT(funcName) {\
		structName * structPtr = ( structName * ) jobParamPtr;\
		type0 & name0 = structPtr->name0;\
		type1 & name1 = structPtr->name1;

#define END_JOB_INLINE() }};

typedef std::atomic<int> JobWaitingCounter;

struct JobDescriptor
{
	JobEntryPoint entryPoint = 0;
	void* dataPtr = 0;
	JobWaitingCounter* counterPtr = 0;
	int stackSize = 0;
	int fixedProcessor = -1;

	JobDescriptor() {}

	JobDescriptor(JobEntryPoint inEntryPoint, void* inDataPtr = 0, int inFixedProcessor = -1)
		: entryPoint(inEntryPoint), dataPtr(inDataPtr), fixedProcessor(inFixedProcessor)
	{}
};

const int gJobSystemWorkerThreadCount = 6;

extern void RunJobSystem(JobDescriptor* startJobDescPtr);
extern void StopJobSystem();
// if waitingCounterPtr != 0, all the jobs created will be add to that counter
extern void RunJobs(JobDescriptor* jobDescPtr, int count = 1, JobWaitingCounter* waitingCounterPtr = 0);
extern JobWaitingCounter* AcquireCounter();
extern void ReleaseCounter(JobWaitingCounter*& waitingCounterPtr);
extern void WaitOnCounter(JobWaitingCounter* waitingCounterPtr, int waitingCounterTarget, bool bAutoRelease = true);
extern int GetCurrentJobProcessor();

extern void AssertFreeFiber();