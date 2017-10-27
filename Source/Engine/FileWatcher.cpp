
#include "windows.h"

#include <iostream>

#include <stdio.h>
#include "FileWatcher.h"


void FileWatcherWorker::Start(const char* path)
{
	hDir = CreateFileA(
		path,
		FILE_LIST_DIRECTORY,
		FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		NULL
		);

	bRun = true;
	threadPtr = new std::thread(&FileWatcherWorker::Run, this);
}

void FileWatcherWorker::Stop()
{
	bRun = false;

	// cancel the ReadDirectoryChanges operation
	CancelIoEx(hDir, NULL);

	// close the directory
	CloseHandle(hDir);

	if (threadPtr)
	{
		threadPtr->join();
		delete threadPtr;
		threadPtr = 0;
	}
}

void FileWatcherWorker::Run()
{
	__declspec(align(128)) unsigned char buffer[32 * 1024];
	FILE_NOTIFY_INFORMATION* fni = (FILE_NOTIFY_INFORMATION*)buffer;
	DWORD size = 0;

	while (bRun)
	{
		if (ReadDirectoryChangesW(hDir, fni, sizeof(buffer), FALSE,
			0
			| FILE_NOTIFY_CHANGE_FILE_NAME
			//			| FILE_NOTIFY_CHANGE_DIR_NAME
			//			| FILE_NOTIFY_CHANGE_ATTRIBUTES
			| FILE_NOTIFY_CHANGE_SIZE
			| FILE_NOTIFY_CHANGE_LAST_WRITE
			//			| FILE_NOTIFY_CHANGE_LAST_ACCESS
			| FILE_NOTIFY_CHANGE_CREATION
			//			| FILE_NOTIFY_CHANGE_SECURITY
			,
			&size, NULL, NULL) == 0)
		{
			printf("Error reading directory changes.\r\n");
		}
		else
		{
			char filename[512];

			FILE_NOTIFY_INFORMATION* next = fni;
			while (1)
			{
				//memcpy(filename, next->FileName, next->FileNameLength);
				//filename[next->FileNameLength / 2] = 0;
				size_t retval;
				wcstombs_s(&retval, filename, next->FileName, next->FileNameLength / 2);

				bool bShouldEnqueue = true;
				FileChangeResult result;
				result.fileName = filename;

				switch (next->Action)
				{
				case FILE_ACTION_ADDED:
					result.type = EFileChangeType::FileAdded;
					break;
				case FILE_ACTION_REMOVED:
					result.type = EFileChangeType::FileRemoved;
					break;
				case FILE_ACTION_MODIFIED:
					result.type = EFileChangeType::FileUpdated;
					break;
				case FILE_ACTION_RENAMED_NEW_NAME:
					result.type = EFileChangeType::FileRenamed;
					break;
				default:
					bShouldEnqueue = false;
					break;
				}

				if (bShouldEnqueue)
				{
					queue.enqueue(result);
				}

				if (next->NextEntryOffset == 0)
					break;

				next = (FILE_NOTIFY_INFORMATION*)(((uintptr_t)next) + next->NextEntryOffset);
			}
		}
	}
}

bool DirectoryExists(const char* dirName)
{
	DWORD ftyp = GetFileAttributesA(dirName);
	if (ftyp == INVALID_FILE_ATTRIBUTES)
		return false;  //something is wrong with your path!

	if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
		return true;   // this is a directory!

	return false;    // this is not a directory!
}

bool FileWatcher::Start(const char* dirName)
{
	char path[512];
	GetFullPathNameA(dirName, sizeof(path), path, 0);

	printf("watch path : %s \n", path);

	if (!DirectoryExists(path))
		return false;

	FileWatcherWorker* worker = new FileWatcherWorker();
	workers.push_back(worker);

	worker->Start(path);

	return true;
}

void FileWatcher::Update(REArray<FileChangeResult>& out)
{
	FileChangeResult result;
	for (int i = 0, ni = (int)workers.size(); i < ni; ++i)
	{
		FileWatcherWorker* worker = workers[i];
		while (worker->queue.dequeue(result))
		{
			out.push_back(result);
		}
	}
}

void FileWatcher::Stop()
{
	for (int i = 0, ni = (int)workers.size(); i < ni; ++i)
	{
		FileWatcherWorker* worker = workers[i];
		worker->Stop();
		delete worker;
	}
	workers.clear();
}