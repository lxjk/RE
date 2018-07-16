#pragma once

#include <thread>
#include <atomic>
#include <functional>

#include "Containers/Containers.h"

#define USE_JOB_SYSTEM 1

typedef void(*JobEntryPoint)(void* customDataPtr);
//typedef std::function<void(void*)> JobEntryPoint;

#define JOB_ENTRY_POINT(funcName) void funcName(void* customDataPtr)
#define JOB_METHOD_ENTRY_POINT(funcName) static void funcName(void* customDataPtr)
#define JOB_LAMBDA_ENTRY_POINT(lambdaName) auto lambdaName = [=](void* customDataPtr)

template<class TLambda>
struct JobLambdaWrapper
{
	const TLambda& func;
	void* dataPtr;

	JobLambdaWrapper(const TLambda& inFunc, void* inDataPtr)
		: func(inFunc), dataPtr(inDataPtr)
	{}

	JOB_METHOD_ENTRY_POINT(Callback)
	{
		JobLambdaWrapper* wrapper = (JobLambdaWrapper*)customDataPtr;
		wrapper->func(wrapper->dataPtr);
	}
};

enum class EJobPriority
{
	High = 0,
	Normal,
	Low,
	// only for init value
	Count,
	// special priority for render thread
	Render = Count,
};


class JobWaitingCounter
{
public:
	__forceinline int Get() { return counter.load(std::memory_order_relaxed); }
	__forceinline void Set(int value) { counter.store(value, std::memory_order_relaxed); }
	__forceinline int Add(int value) { return counter.fetch_add(value, std::memory_order_relaxed); }
	__forceinline int Sub(int value) { return counter.fetch_sub(value, std::memory_order_relaxed); }
protected:
	std::atomic<int> counter = 0;
};

struct JobDescriptor
{
	JobEntryPoint entryPoint = 0;
	void* dataPtr = 0;
	int stackSize = 0;
	EJobPriority priority = EJobPriority::Normal;

	// counter is assigned by job system call
	JobWaitingCounter* counterPtr = 0;

	JobDescriptor() {}

	JobDescriptor(const JobEntryPoint& inEntryPoint, void* inDataPtr = 0, EJobPriority inPriority = EJobPriority::Normal)
		: entryPoint(inEntryPoint), dataPtr(inDataPtr), priority(inPriority)
	{}
};

#define RUN_INLINE_JOB(priority, counterPtr, inCustomDataPtr, code) \
{\
JOB_LAMBDA_ENTRY_POINT(lambda) { code };\
JobLambdaWrapper<decltype(lambda)> wrapper(lambda, inCustomDataPtr);\
JobDescriptor desc(&decltype(wrapper)::Callback, &wrapper, priority);\
RunJobs(&desc, 1, counterPtr);\
}

// will block current job until this new job is done (same as wait on a new counter)
#define RUN_INLINE_JOB_BLOCK(priority, inCustomDataPtr, code) \
{\
JobWaitingCounter counter;\
RUN_INLINE_JOB(priority, &counter, inCustomDataPtr, code);\
WaitOnCounter(&counter);\
}



const int gJobSystemWorkerThreadCount = 6;
const int gRenderProcessorIndex = 0;

extern void RunJobSystem(JobDescriptor* startJobDescPtr);
extern void StopJobSystem();
// if waitingCounterPtr != 0, all the jobs created will be add to that counter
extern void RunJobs(JobDescriptor* jobDescPtr, int count = 1, JobWaitingCounter* waitingCounterPtr = 0);
extern JobWaitingCounter* AcquireCounter();
extern void ReleaseCounter(JobWaitingCounter*& waitingCounterPtr);
extern void WaitOnCounter(JobWaitingCounter* waitingCounterPtr, int waitingCounterTarget = 0);
extern int GetCurrentJobProcessor();

extern void AssertFreeFiber();