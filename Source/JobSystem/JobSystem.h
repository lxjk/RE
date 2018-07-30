#pragma once

#include <thread>
#include <atomic>
#include <functional>

#include "Containers/Containers.h"

#define USE_JOB_SYSTEM 1

const int gJobSystemWorkerThreadCount = 6;
const int gRenderProcessorIndex = gJobSystemWorkerThreadCount - 1;

typedef void(*JobEntryPoint)(void* customDataPtr);
//typedef std::function<void(void*)> JobEntryPoint;

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

extern JobWaitingCounter gRenderFrameSyncCounter;

#define JOB_ENTRY_POINT(funcName) void funcName(void* customDataPtr)
#define JOB_METHOD_ENTRY_POINT(funcName) static void funcName(void* customDataPtr)
#define JOB_LAMBDA_ENTRY_POINT(lambdaName, ...) auto lambdaName = [=, __VA_ARGS__](void* customDataPtr)
#define JOB_LAMBDA_REF_ENTRY_POINT(lambdaName, ...) auto lambdaName = [&, __VA_ARGS__](void* customDataPtr)

template<class TLambda>
struct JobLambdaWrapper
{
	const TLambda func;
	void* dataPtr;
	bool bHeap;

	JobLambdaWrapper(const TLambda& inFunc, void* inDataPtr)
		: func(std::move(inFunc)), dataPtr(inDataPtr), bHeap(false)
	{}

	static JobLambdaWrapper* Create(const TLambda& inFunc, void* inDataPtr)
	{
		JobLambdaWrapper* wrapper = new JobLambdaWrapper<TLambda>(inFunc, inDataPtr);
		wrapper->bHeap = true;
		return wrapper;
	}

	JOB_METHOD_ENTRY_POINT(Callback)
	{
		JobLambdaWrapper* wrapper = (JobLambdaWrapper*)customDataPtr;
		wrapper->func(wrapper->dataPtr);
		if (wrapper->bHeap)
			delete wrapper;
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

extern void RunJobSystem(JobDescriptor* startJobDescPtr);
extern void StopJobSystem();
// if waitingCounterPtr != 0, all the jobs created will be add to that counter
extern void RunJobs(JobDescriptor* jobDescPtr, int count = 1, JobWaitingCounter* waitingCounterPtr = 0);
extern void WaitOnCounter(JobWaitingCounter* waitingCounterPtr, int waitingCounterTarget = 0);
extern int GetCurrentJobProcessor();

extern void AssertFreeFiber();


#define RUN_INLINE_JOB(priority, counterPtr, inCustomDataPtr, code, ...) \
{\
JOB_LAMBDA_ENTRY_POINT(lambda, __VA_ARGS__) { code };\
auto wrapperPtr = JobLambdaWrapper<decltype(lambda)>::Create(lambda, inCustomDataPtr);\
JobDescriptor desc(&JobLambdaWrapper<decltype(lambda)>::Callback, wrapperPtr, priority);\
RunJobs(&desc, 1, counterPtr);\
}

// will block current job until this new job is done (same as wait on a new counter)
#define RUN_INLINE_JOB_BLOCK(priority, inCustomDataPtr, code, ...) \
{\
JOB_LAMBDA_REF_ENTRY_POINT(lambda, __VA_ARGS__) { code };\
JobLambdaWrapper<decltype(lambda)> wrapper(lambda, inCustomDataPtr);\
JobDescriptor desc(&JobLambdaWrapper<decltype(lambda)>::Callback, &wrapper, priority);\
JobWaitingCounter counter;\
RunJobs(&desc, 1, counterPtr);\
WaitOnCounter(&counter);\
}

#if USE_JOB_SYSTEM
#define RUN_INLINE_RENDER_JOB(code, ...) \
{\
JOB_LAMBDA_ENTRY_POINT(lambda, __VA_ARGS__) { code };\
if(GetCurrentJobProcessor() == gRenderProcessorIndex)\
{\
	lambda(0);\
}\
else\
{\
	auto wrapperPtr = JobLambdaWrapper<decltype(lambda)>::Create(lambda, 0);\
	JobDescriptor desc(&JobLambdaWrapper<decltype(lambda)>::Callback, wrapperPtr, EJobPriority::Render);\
	RunJobs(&desc, 1, &gRenderFrameSyncCounter);\
}\
}

#define RUN_INLINE_RENDER_JOB_BLOCK(code, ...) \
{\
JOB_LAMBDA_REF_ENTRY_POINT(lambda, __VA_ARGS__) { code };\
if(GetCurrentJobProcessor() == gRenderProcessorIndex)\
{\
	lambda(0);\
}\
else\
{\
	JobLambdaWrapper<decltype(lambda)> wrapper(lambda, 0);\
	JobDescriptor desc(&JobLambdaWrapper<decltype(lambda)>::Callback, &wrapper, EJobPriority::Render);\
	JobWaitingCounter counter;\
	RunJobs(&desc, 1, &counter);\
	WaitOnCounter(&counter);\
}\
}
#else
#define RUN_INLINE_RENDER_JOB(code, ...) code
#define RUN_INLINE_RENDER_JOB_BLOCK(code, ...) code
#endif