#pragma once

#include "spsc.h"

#include <string>
#include <vector>
#include <thread>

enum EFileChangeType
{
	FileAdded,
	FileRemoved,
	FileUpdated,
	FileRenamed
};

struct FileChangeResult
{
	std::string fileName;
	EFileChangeType type;
};

class FileWatcherWorker
{
public:

	void Start(const char* path);
	void Run();
	void Stop();

	FileWatcherWorker()
	{
		bRun = false;
		threadPtr = 0;
	}

	bool bRun = false;
	std::thread* threadPtr = 0;
	void* hDir;
	spsc_queue<FileChangeResult> queue;
};

class FileWatcher
{
public:

	bool Start(const char* dirName);

	void Update(std::vector<FileChangeResult>& out);

	void Stop();

protected:

	std::vector<FileWatcherWorker*> workers;
};