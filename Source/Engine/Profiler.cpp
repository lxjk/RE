#include "Profiler.h"


std::map<std::string, double> ScopedProfileTimerCPU::timerMap;
std::unordered_map<std::string, double> ScopedProfileTimerCPU::timeStampMap[2];
int ScopedProfileTimerCPU::MapWriteIdx = 0;
char ScopedProfileTimerCPU::fullName[1024];


std::map<std::string, double> ScopedProfileTimerGPU::timerMap;
std::unordered_map<std::string, std::vector<TimestampPair>> ScopedProfileTimerGPU::timeStampPairMap[2];
int ScopedProfileTimerGPU::MapWriteIdx = 0;
char ScopedProfileTimerGPU::fullName[1024];