
#include <stdlib.h>
#include <cassert>
#include "Windows.h"

#include "JobSystem.h"


struct JobFiberData
{
	JobEntryPoint jobEntryPoint = 0;
	void* jobDataPtr = 0;
	void* schedulerFiber = 0;
	JobWaitingCounter* counterPtr = 0;
	JobWaitingCounter* waitingCounterPtr = 0;
	int waitingCounterTarget = 0;
	bool bAutoReleaseWaitingCounter = true;
	int currentCore = 0;
	int fixedCore = -1; // -1 means not fixed

	void Reset()
	{
		jobEntryPoint = 0;
		jobDataPtr = 0;
		schedulerFiber = 0;
		counterPtr = 0;
		waitingCounterPtr = 0;
		waitingCounterTarget = 0;
		bAutoReleaseWaitingCounter = true;
		currentCore = 0;
		fixedCore = -1;
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
	int coreIndex = 0;
};

bool bJobSystemRuning = false;

REArray<JobSystemWorker> gJobSystemWorkerList;

REQueue<JobDescriptor> gJobQueue;

REArray<JobFiberListData> gFreeFiberList;
REArray<JobFiberListData> gWaitingFiberList;

// spin locks
std::atomic_flag gJobQueueLock = ATOMIC_FLAG_INIT;
std::atomic_flag gFreeFiberListLock = ATOMIC_FLAG_INIT;
std::atomic_flag gWaitingFiberListLock = ATOMIC_FLAG_INIT;

__forceinline void AcquireSpinLock(std::atomic_flag& lock)
{
	while (lock.test_and_set(std::memory_order_acquire))
		;
}

__forceinline void ReleaseSpinLock(std::atomic_flag& lock)
{
	lock.clear(std::memory_order_release);
}


void GetNextJobFiber(void* schedulerFiber, int coreIndex, void*& outFiber, JobFiberData*& outFiberDataPtr)
{
	outFiber = 0;
	outFiberDataPtr = 0;

	JobFiberListData nextFiberData;

	// check waiting list first
	AcquireSpinLock(gWaitingFiberListLock);

	for (int i = 0, ni = (int)gWaitingFiberList.size(); i < ni; ++i)
	{
		const JobFiberListData& waitingData = gWaitingFiberList[i];
		JobFiberData* fiberDataPtr = waitingData.fiberDataPtr;
		if ((!fiberDataPtr->waitingCounterPtr || (*fiberDataPtr->waitingCounterPtr).load() == fiberDataPtr->waitingCounterTarget) &&
			(fiberDataPtr->fixedCore < 0 || fiberDataPtr->fixedCore == coreIndex))
		{
			// assign fiber data
			nextFiberData = waitingData;
			// remove from list
			gWaitingFiberList.erase(gWaitingFiberList.begin() + i);
			break;
		}
	}

	ReleaseSpinLock(gWaitingFiberListLock);

	if (nextFiberData.fiber)
	{
		assert(nextFiberData.fiberDataPtr);
		nextFiberData.fiberDataPtr->schedulerFiber = schedulerFiber;
		nextFiberData.fiberDataPtr->currentCore = coreIndex;
		// release counter
		if (nextFiberData.fiberDataPtr->waitingCounterPtr && nextFiberData.fiberDataPtr->bAutoReleaseWaitingCounter)
		{
			delete nextFiberData.fiberDataPtr->waitingCounterPtr;
			nextFiberData.fiberDataPtr->waitingCounterPtr = 0;
		}
		outFiber = nextFiberData.fiber;
		outFiberDataPtr = nextFiberData.fiberDataPtr;

		//printf("pick up waiting list thread ID: %x, current processor: %d\n", GetCurrentThreadId(), GetCurrentProcessorNumber());

		return;
	}

	// no waiting fiber, try get a free fiber and a new job
	JobDescriptor jobDesc;

	AcquireSpinLock(gFreeFiberListLock);

	if (!gFreeFiberList.empty())
	{
		AcquireSpinLock(gJobQueueLock);

		// get job
		if (!gJobQueue.empty())
		{
			// assign job desc
			jobDesc = gJobQueue.front();
			// remove job from queue
			gJobQueue.pop();
			// assign fiber
			nextFiberData = gFreeFiberList.back();
			// remove fiber
			gFreeFiberList.pop_back();
		}

		ReleaseSpinLock(gJobQueueLock);
	}

	ReleaseSpinLock(gFreeFiberListLock);

	// we have a valid job to execute
	if (nextFiberData.fiber)
	{
		assert(nextFiberData.fiberDataPtr);

		nextFiberData.fiberDataPtr->Reset();
		nextFiberData.fiberDataPtr->jobEntryPoint = jobDesc.entryPoint;
		nextFiberData.fiberDataPtr->jobDataPtr = jobDesc.dataPtr;
		nextFiberData.fiberDataPtr->counterPtr = jobDesc.counterPtr;
		nextFiberData.fiberDataPtr->schedulerFiber = schedulerFiber;
		nextFiberData.fiberDataPtr->currentCore = coreIndex;
		nextFiberData.fiberDataPtr->fixedCore = jobDesc.fixedCore;

		outFiber = nextFiberData.fiber;
		outFiberDataPtr = nextFiberData.fiberDataPtr;
	}
}

void AddJobFiberToFreeList(void* fiber, void* dataPtr)
{
	assert(fiber);

	JobFiberListData fiberData;
	fiberData.fiber = fiber;
	fiberData.fiberDataPtr = (JobFiberData*)dataPtr;

	AcquireSpinLock(gFreeFiberListLock);

	gFreeFiberList.push_back(fiberData);

	ReleaseSpinLock(gFreeFiberListLock);
}

void AddJobFiberToWaitingList(void* fiber, void* dataPtr)
{
	assert(fiber);
	assert(dataPtr);

	JobFiberListData fiberData;
	fiberData.fiber = fiber;
	fiberData.fiberDataPtr = (JobFiberData*)dataPtr;

	AcquireSpinLock(gWaitingFiberListLock);

	gWaitingFiberList.push_back(fiberData);

	ReleaseSpinLock(gWaitingFiberListLock);
}

void JobSystemWorker::Start(int inCoreIndex)
{
	coreIndex = inCoreIndex;
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
	SetThreadAffinityMask(GetCurrentThread(), 1i64 << coreIndex);
	printf("START: thread ID: %x, current processor: %d\n", GetCurrentThreadId(), coreIndex);
	fiber = ConvertThreadToFiber(0);
	assert(fiber);

	while (bJobSystemRuning)
	{
		//printf("thread ID: %x, current processor: %d\n", GetCurrentThreadId(), GetCurrentProcessorNumber());

		// pull jobs
		{
			void* nextJobFiber = 0;
			JobFiberData* nextJobFiberDataPtr = 0;
			GetNextJobFiber(fiber, coreIndex, nextJobFiber, nextJobFiberDataPtr);
			if (nextJobFiber)
			{
				//printf("Pick up job: thread ID: %x, current processor: %d\n", 
				//	GetCurrentThreadId(), GetCurrentProcessorNumber());

				bool bCoreMatch = true;
				if (nextJobFiberDataPtr->fixedCore < 0 || nextJobFiberDataPtr->fixedCore == coreIndex)
					SwitchToFiber(nextJobFiber);
				else
					bCoreMatch = false;

				// we got switched back, now add the fiber to either free list or waiting list
				// OR the job we got doesn't match our core index, just add it to waiting list
				if (nextJobFiberDataPtr->waitingCounterPtr || !bCoreMatch)
				{
					AddJobFiberToWaitingList(nextJobFiber, nextJobFiberDataPtr);
				}
				else
				{
					// we are done, return to free list
					AddJobFiberToFreeList(nextJobFiber, nextJobFiberDataPtr);
				}
			}
			else
				Sleep(1);
		}
	}
}


void __stdcall JobFiberFunc(void* lpParameter)
{

	while (1)
	{
		JobFiberData* fiberDataPtr = (JobFiberData*)GetFiberData();

		assert(fiberDataPtr->jobEntryPoint);
		assert(fiberDataPtr->schedulerFiber);

		// execute user function
		fiberDataPtr->jobEntryPoint(fiberDataPtr->jobDataPtr);
		
		// decrement counter
		if (fiberDataPtr->counterPtr)
		{
			--(*fiberDataPtr->counterPtr);
		}

		// switch back to sheduler fiber
		SwitchToFiber(fiberDataPtr->schedulerFiber);
	}
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
	gFreeFiberList.resize(fiberCount);
	for(int i = 0; i < fiberCount; ++i)
	{
		gFreeFiberList[i].fiberDataPtr = new JobFiberData();
		gFreeFiberList[i].fiber = CreateFiber(0, &JobFiberFunc, gFreeFiberList[i].fiberDataPtr);
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
	gJobSystemWorkerList[0].coreIndex = 0;

	// add start job
	RunJobs(startJobDescPtr);

	// start runing on job system for this thread
	gJobSystemWorkerList[0].Run();

	// the job system is stopped now, do clean up for all threads we spawned
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
		waitingCounterPtr->fetch_add(count);
		//printf("RunJobs: counter %d\n", counter->load());
	}

	AcquireSpinLock(gJobQueueLock);

	for (int i = 0; i < count; ++i)
	{
		gJobQueue.push(*jobDescPtr);
		gJobQueue.back().counterPtr = waitingCounterPtr;
		++jobDescPtr;
	}

	ReleaseSpinLock(gJobQueueLock);
#else
	for (int i = 0; i < count; ++i)
	{
		if (jobDescPtr && jobDescPtr->entryPoint)
			jobDescPtr->entryPoint(jobDescPtr->dataPtr);
		++jobDescPtr;
	}
#endif
}

JobWaitingCounter* AcquireCounter()
{
	JobWaitingCounter* counter = new JobWaitingCounter();
	counter->store(0);
	return counter;
}

void ReleaseCounter(JobWaitingCounter*& waitingCounterPtr)
{
	if (waitingCounterPtr)
	{
		delete waitingCounterPtr;
		waitingCounterPtr = 0;
	}
}


void WaitOnCounter(JobWaitingCounter* waitingCounterPtr, int waitingCounterTarget, bool bAutoRelease)
{
#if USE_JOB_SYSTEM
	if (waitingCounterPtr && waitingCounterPtr->load() == waitingCounterTarget)
	{
		// we already reached the goal, simply resume
		if (bAutoRelease)
			delete waitingCounterPtr;

		return;
	}

	JobFiberData* fiberDataPtr = (JobFiberData*)GetFiberData();
	fiberDataPtr->waitingCounterPtr = waitingCounterPtr;
	fiberDataPtr->waitingCounterTarget = waitingCounterTarget;
	fiberDataPtr->bAutoReleaseWaitingCounter = bAutoRelease;

	//printf("WaitOnAndFreeCounter thread ID: %x, current processor: %d\n", GetCurrentThreadId(), GetCurrentProcessorNumber());

	// switch back to sheduler fiber
	SwitchToFiber(fiberDataPtr->schedulerFiber);
#else
	if(waitingCounterPtr && bAutoRelease)
		delete waitingCounterPtr;
#endif
}

int GetCurrentCore()
{
#if USE_JOB_SYSTEM
	JobFiberData* fiberDataPtr = (JobFiberData*)GetFiberData();
	return fiberDataPtr->currentCore;
#else
	return 0;
#endif
}