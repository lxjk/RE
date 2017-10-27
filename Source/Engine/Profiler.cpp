#include "Profiler.h"


RESortedMap<std::string, double> ScopedProfileTimerCPU::timerMap;
REMap<std::string, double> ScopedProfileTimerCPU::timeStampMap[2];
int ScopedProfileTimerCPU::MapWriteIdx = 0;
char ScopedProfileTimerCPU::fullName[1024];


RESortedMap<std::string, double> ScopedProfileTimerGPU::timerMap;
REMap<std::string, REArray<TimestampPair>> ScopedProfileTimerGPU::timeStampPairMap[2];
int ScopedProfileTimerGPU::MapWriteIdx = 0;
char ScopedProfileTimerGPU::fullName[1024];