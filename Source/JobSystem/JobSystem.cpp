
#include <stdlib.h>
#include <cassert>
#include "Windows.h"

#include "Locks.h"
#include "JobSystem.h"


struct JobFiberData
{
	JobEntryPoint jobEntryPoint = 0;
	void* jobDataPtr = 0;
	void* prevFiber;
	JobFiberData* prevFiberDataPtr;
	JobWaitingCounter* counterPtr = 0;
	JobWaitingCounter* waitingCounterPtr = 0;
	int waitingCounterTarget = 0;
	int currentProcessor = 0;
	int fixedProcessor = -1; // -1 means not fixed

	void Reset()
	{
		jobEntryPoint = 0;
		jobDataPtr = 0;
		prevFiber = 0;
		prevFiberDataPtr = 0;
		counterPtr = 0;
		waitingCounterPtr = 0;
		waitingCounterTarget = 0;
		currentProcessor = 0;
		fixedProcessor = -1;
	}
};

struct JobFiberListData
{
	void* fiber = 0;
	JobFiberData* fiberDataPtr = 0;
};

class JobSystemWorker
{
public:
	void Start(int inCoreIndex);
	void Run();
	void Stop();

	std::thread* threadPtr = 0;
	void* fiber = 0;
	int processorIndex = 0;
};

bool bJobSystemRuning = false;

REArray<JobSystemWorker> gJobSystemWorkerList;

REQueue<JobDescriptor> gJobQueue[(int)EJobPriority::Count];
REQueue<JobDescriptor> gRenderJobQueue;

ThreadProtected<REArray<JobFiberListData>, SpinLock> gFreeFiberList;
ThreadProtected<REArray<JobFiberListData>, SpinLock> gWaitingFiberList;

// spin locks
SpinLock gJobQueueLock;

__forceinline bool CheckWaitingCounter(JobWaitingCounter* waitingCounterPtr, int waitingCounterTarget)
{
	if (waitingCounterPtr && waitingCounterPtr->Get() == waitingCounterTarget)
	{
		return true;
	}
	return false;
}

__forceinline bool CheckWaitingCounter(JobFiberData* fiberDataPtr)
{
	return CheckWaitingCounter(fiberDataPtr->waitingCounterPtr, fiberDataPtr->waitingCounterTarget);
}


void AddJobFiberToFreeList(void* fiber, void* dataPtr)
{
	assert(fiber);

	JobFiberListData fiberData;
	fiberData.fiber = fiber;
	fiberData.fiberDataPtr = (JobFiberData*)dataPtr;
	
	{
		SCOPED_READ_WRITE_REF(gFreeFiberList, gFreeFiberListRef)
		gFreeFiberListRef.push_back(fiberData);
	}

}

void AddJobFiberToWaitingList(void* fiber, void* dataPtr)
{
	assert(fiber);
	assert(dataPtr);

	JobFiberListData fiberData;
	fiberData.fiber = fiber;
	fiberData.fiberDataPtr = (JobFiberData*)dataPtr;

	{
		SCOPED_READ_WRITE_REF(gWaitingFiberList, gWaitingFiberListRef)
		gWaitingFiberListRef.push_back(fiberData);
	}
}

void GetNextJobFiber(void* prevFiber, JobFiberData* prevFiberDataPtr, int processorIndex, 
	void*& outFiber, JobFiberData*& outFiberDataPtr)
{
	outFiber = 0;
	outFiberDataPtr = 0;

	JobFiberListData nextFiberData;

	//printf("GetNextJobFiber thread ID: %x, current processor: %d\n", GetCurrentThreadId(), GetCurrentProcessorNumber());

	// check waiting list first
	{
		SCOPED_READ_WRITE_REF(gWaitingFiberList, gWaitingFiberListRef)

		for (int i = 0, ni = (int)gWaitingFiberListRef.size(); i < ni; ++i)
		{
			const JobFiberListData& waitingData = gWaitingFiberListRef[i];
			JobFiberData* fiberDataPtr = waitingData.fiberDataPtr;
			if ((!fiberDataPtr->waitingCounterPtr || (*fiberDataPtr->waitingCounterPtr).Get() == fiberDataPtr->waitingCounterTarget) &&
				(fiberDataPtr->fixedProcessor < 0 || fiberDataPtr->fixedProcessor == processorIndex))
			{
				// assign fiber data
				nextFiberData = waitingData;
				// remove from list
				gWaitingFiberListRef.erase(gWaitingFiberListRef.begin() + i);
				break;
			}
		}
	}

	if (nextFiberData.fiber)
	{
		assert(nextFiberData.fiberDataPtr);
		nextFiberData.fiberDataPtr->prevFiber = prevFiber;
		nextFiberData.fiberDataPtr->prevFiberDataPtr = prevFiberDataPtr;
		nextFiberData.fiberDataPtr->currentProcessor = processorIndex;
		// clear waiting status
		nextFiberData.fiberDataPtr->waitingCounterPtr = 0;
		nextFiberData.fiberDataPtr->waitingCounterTarget = 0;
		outFiber = nextFiberData.fiber;
		outFiberDataPtr = nextFiberData.fiberDataPtr;

		//printf("pick up waiting list thread ID: %x, current processor: %d\n", GetCurrentThreadId(), GetCurrentProcessorNumber());

		return;
	}

	// no waiting fiber, try get a free fiber and a new job
	JobDescriptor jobDesc;

	bool bCanUsePrevFiber = (prevFiber && prevFiberDataPtr && !prevFiberDataPtr->waitingCounterPtr);

	if (bCanUsePrevFiber)
	{
		gJobQueueLock.LockReadWrite();

		//printf("CanUsePrevFiber thread ID: %x, current processor: %d\n", GetCurrentThreadId(), GetCurrentProcessorNumber());

		// get job
		REQueue<JobDescriptor>* targetQueuePtr = 0;
		if (processorIndex == gRenderProcessorIndex)
		{
			if (!gRenderJobQueue.empty())
				targetQueuePtr = &gRenderJobQueue;
		}
		else
		{
			for (int i = 0; i < (int)EJobPriority::Count; ++i)
			{
				if (!gJobQueue[i].empty())
				{
					targetQueuePtr = &gJobQueue[i];
					break;
				}
			}
		}
		if (targetQueuePtr)
		{
			// assign job desc
			jobDesc = targetQueuePtr->front();
			// remove job from queue
			targetQueuePtr->pop();
			// assign fiber
			nextFiberData.fiber = prevFiber;
			nextFiberData.fiberDataPtr = prevFiberDataPtr;
		}

		gJobQueueLock.UnlockReadWrite();
	}
	else
	{
		SCOPED_READ_WRITE_REF(gFreeFiberList, gFreeFiberListRef);
		
		if (!gFreeFiberListRef.empty())
		{
			gJobQueueLock.LockReadWrite();

			// get job
			REQueue<JobDescriptor>* targetQueuePtr = 0;
			if (processorIndex == gRenderProcessorIndex)
			{
				if (!gRenderJobQueue.empty())
					targetQueuePtr = &gRenderJobQueue;
			}
			else
			{
				for (int i = 0; i < (int)EJobPriority::Count; ++i)
				{
					if (!gJobQueue[i].empty())
					{
						targetQueuePtr = &gJobQueue[i];
						break;
					}
				}
			}
			if (targetQueuePtr)
			{
				// assign job desc
				jobDesc = targetQueuePtr->front();
				// remove job from queue
				targetQueuePtr->pop();
				// assign fiber
				nextFiberData = gFreeFiberListRef.back();
				// remove fiber
				gFreeFiberListRef.pop_back();
			}

			gJobQueueLock.UnlockReadWrite();
		}
	}

	// we have a valid job to execute
	if (nextFiberData.fiber)
	{
		assert(nextFiberData.fiberDataPtr);

		nextFiberData.fiberDataPtr->Reset();
		nextFiberData.fiberDataPtr->jobEntryPoint = jobDesc.entryPoint;
		nextFiberData.fiberDataPtr->jobDataPtr = jobDesc.dataPtr;
		nextFiberData.fiberDataPtr->counterPtr = jobDesc.counterPtr;
		nextFiberData.fiberDataPtr->prevFiber = prevFiber;
		nextFiberData.fiberDataPtr->prevFiberDataPtr = prevFiberDataPtr;
		nextFiberData.fiberDataPtr->currentProcessor = processorIndex;
		nextFiberData.fiberDataPtr->fixedProcessor = (jobDesc.priority == EJobPriority::Render) ? gRenderProcessorIndex : -1;

		outFiber = nextFiberData.fiber;
		outFiberDataPtr = nextFiberData.fiberDataPtr;

		//printf("pick up free list thread ID: %x, current processor: %d\n", GetCurrentThreadId(), GetCurrentProcessorNumber());
	}
}

void PullNextJob(void* fiber, JobFiberData* fiberDataPtr, int processorIndex)
{
	// pull next job
	void* nextJobFiber = 0;
	JobFiberData* nextJobFiberDataPtr = 0;

	while (bJobSystemRuning && !nextJobFiber)
	{
		// check waiting counter
		if (fiberDataPtr && CheckWaitingCounter(fiberDataPtr))
			return;

		GetNextJobFiber(fiber, fiberDataPtr, processorIndex, nextJobFiber, nextJobFiberDataPtr);
		if (nextJobFiber)
		{
			// core doesn't match, return to the waiting list
			if (nextJobFiberDataPtr->fixedProcessor >= 0 && nextJobFiberDataPtr->fixedProcessor != processorIndex)
			{
				AddJobFiberToWaitingList(nextJobFiber, nextJobFiberDataPtr);
				// clear job fiber, try again
				nextJobFiber = 0;
				nextJobFiberDataPtr = 0;
			}
		}
		else
			std::this_thread::yield();
			//Sleep(1);
	}

	// we got a different fiber, switch
	if (nextJobFiber && nextJobFiber != fiber)
	{
		//printf("before switch fiber thread ID: %x, current processor: %d, %x->%x\n", GetCurrentThreadId(), GetCurrentProcessorNumber(), fiber, nextJobFiber);
		SwitchToFiber(nextJobFiber);
		//printf("after switch fiber thread ID: %x, current processor: %d\n", GetCurrentThreadId(), GetCurrentProcessorNumber());
	}
}

void ReturnPrevFiber(void* fiber, JobFiberData* fiberDataPtr)
{
	if (fiberDataPtr->prevFiber && fiberDataPtr->prevFiber != fiber)
	{
		assert(fiberDataPtr->prevFiberDataPtr);

		if (fiberDataPtr->prevFiberDataPtr->waitingCounterPtr)
		{
			//printf("return fiber to waiting thread ID: %x, current processor: %d\n", GetCurrentThreadId(), GetCurrentProcessorNumber());
			AddJobFiberToWaitingList(fiberDataPtr->prevFiber, fiberDataPtr->prevFiberDataPtr);
		}
		else
		{
			//printf("return fiber to free thread ID: %x, current processor: %d\n", GetCurrentThreadId(), GetCurrentProcessorNumber());
			// we are done, return to free list
			AddJobFiberToFreeList(fiberDataPtr->prevFiber, fiberDataPtr->prevFiberDataPtr);
		}
	}
	// after we return it, clear prevFiber
	fiberDataPtr->prevFiber = 0;
	fiberDataPtr->prevFiberDataPtr = 0;
}

void JobSystemWorker::Start(int inCoreIndex)
{
	processorIndex = inCoreIndex;
	threadPtr = new std::thread(&JobSystemWorker::Run, this);
}

void JobSystemWorker::Stop()
{
	if (threadPtr)
	{
		threadPtr->join();
		delete threadPtr;
		threadPtr = 0;
	}
}

void JobSystemWorker::Run()
{
	SetThreadAffinityMask(GetCurrentThread(), 1i64 << processorIndex);
	printf("START: thread ID: %x, current processor: %d\n", GetCurrentThreadId(), processorIndex);
	fiber = ConvertThreadToFiber(0);
	assert(fiber);

	// start pulling next job
	PullNextJob(0, 0, processorIndex);
}


void __stdcall JobFiberFunc(void* lpParameter)
{
	JobFiberData* fiberDataPtr = (JobFiberData*)lpParameter;
	void* fiber = GetCurrentFiber();

	while (bJobSystemRuning)
	{
		assert(fiberDataPtr->jobEntryPoint);

		// if we came from another fiber, return previous fiber
		ReturnPrevFiber(fiber, fiberDataPtr);

		// execute user function
		fiberDataPtr->jobEntryPoint(fiberDataPtr->jobDataPtr);

		//printf("finish job thread ID: %x, current processor: %d\n", GetCurrentThreadId(), GetCurrentProcessorNumber());

		
		// decrement counter
		if (fiberDataPtr->counterPtr)
		{
			//printf("sub counter: %d thread ID: %x, current processor: %d\n", fiberDataPtr->counterPtr->load(), GetCurrentThreadId(), GetCurrentProcessorNumber());
			fiberDataPtr->counterPtr->Sub(1);
		}

		// pull the next job
		PullNextJob(fiber, fiberDataPtr, fiberDataPtr->currentProcessor);
	}

	SwitchToFiber(gJobSystemWorkerList[fiberDataPtr->currentProcessor].fiber);
}

void RunJobSystem(JobDescriptor* startJobDescPtr)
{
#if USE_JOB_SYSTEM
	assert(gJobSystemWorkerList.size() == 0);

	gJobSystemWorkerList.resize(gJobSystemWorkerThreadCount);

	// set current thread affinity to be first core
	SetThreadAffinityMask(GetCurrentThread(), 1);

	// create fiber list
	const int fiberCount = 32;
	{
		SCOPED_READ_WRITE_REF(gFreeFiberList, gFreeFiberListRef)
		gFreeFiberListRef.resize(fiberCount);
		for (int i = 0; i < fiberCount; ++i)
		{
			gFreeFiberListRef[i].fiberDataPtr = new JobFiberData();
			gFreeFiberListRef[i].fiber = CreateFiber(0, &JobFiberFunc, gFreeFiberListRef[i].fiberDataPtr);
		}
	}

	// start running
	bJobSystemRuning = true;

	// start all worker thread
	// reserve first worker for current thread
	for (int i = 1; i < gJobSystemWorkerThreadCount; ++i)
	{
		gJobSystemWorkerList[i].Start(i);
	}

	// get ready to run fiber on this thread as well
	gJobSystemWorkerList[0].processorIndex = 0;

	// add start job
	RunJobs(startJobDescPtr);

	// start runing on job system for this thread
	gJobSystemWorkerList[0].Run();

	// when we are back here the job system is stopped now, do clean up for all threads we spawned
	for (int i = 1; i < gJobSystemWorkerThreadCount; ++i)
	{
		gJobSystemWorkerList[i].Stop();
	}
#else
	if (startJobDescPtr && startJobDescPtr->entryPoint)
	{
		startJobDescPtr->entryPoint(startJobDescPtr->dataPtr);
	}
#endif
}

void StopJobSystem()
{
	bJobSystemRuning = false;
}

void RunJobs(JobDescriptor* jobDescPtr, int count, JobWaitingCounter* waitingCounterPtr)
{
#if USE_JOB_SYSTEM
	if (waitingCounterPtr)
	{
		waitingCounterPtr->Add(count);
		//printf("RunJobs: counter %d\n", counter->load());
	}

	gJobQueueLock.LockReadWrite();

	//printf("add job thread ID: %x, current processor: %d\n", GetCurrentThreadId(), GetCurrentProcessorNumber());
	
	for (int i = 0; i < count; ++i)
	{
		REQueue<JobDescriptor>& targetQueue =
			(jobDescPtr->priority == EJobPriority::Render) ? gRenderJobQueue : gJobQueue[(int)jobDescPtr->priority];
		targetQueue.push(*jobDescPtr);
		targetQueue.back().counterPtr = waitingCounterPtr;
		++jobDescPtr;
	}

	gJobQueueLock.UnlockReadWrite();
#else
	for (int i = 0; i < count; ++i)
	{
		if (jobDescPtr && jobDescPtr->entryPoint)
			jobDescPtr->entryPoint(jobDescPtr->dataPtr);
		++jobDescPtr;
	}
#endif
}

void WaitOnCounter(JobWaitingCounter* waitingCounterPtr, int waitingCounterTarget)
{
#if USE_JOB_SYSTEM
	if (CheckWaitingCounter(waitingCounterPtr, waitingCounterTarget))
		return;

	JobFiberData* fiberDataPtr = (JobFiberData*)GetFiberData();
	fiberDataPtr->waitingCounterPtr = waitingCounterPtr;
	fiberDataPtr->waitingCounterTarget = waitingCounterTarget;

	void* fiber = GetCurrentFiber();

	//printf("WaitOnAndFreeCounter thread ID: %x, current processor: %d\n", GetCurrentThreadId(), GetCurrentProcessorNumber());

	// pull next job
	PullNextJob(fiber, fiberDataPtr, fiberDataPtr->currentProcessor);

	// check if we need to quit
	if(!bJobSystemRuning)
		SwitchToFiber(gJobSystemWorkerList[fiberDataPtr->currentProcessor].fiber);

	// if we came back from another fiber, return previous fiber
	ReturnPrevFiber(fiber, fiberDataPtr);
#endif
}

int GetCurrentJobProcessor()
{
#if USE_JOB_SYSTEM
	JobFiberData* fiberDataPtr = (JobFiberData*)GetFiberData();
	return fiberDataPtr->currentProcessor;
#else
	return 0;
#endif
}

void AssertFreeFiber()
{
	//assert(gFreeFiberList.GetReadWriteScope().Get().size() == 26);
}