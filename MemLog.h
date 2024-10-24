#pragma once
enum EVENT
{
	J_ON_RELEASE,
	J_ON_ACCEPT,
	ON_ACCEPT,
	ON_RELEASE,
	RELEASE_SESSION,
	SENDPACKET_RELEASE_TRY_1,
	SENDPACKET_RELEASE_TRY_2,
	SENDPACKET_RELEASE_TRY_3,
};

struct MemLog
{
	EVENT event;
	unsigned long long Cnt;
	DWORD threadId;
	unsigned long long sessionID;
	WORD PlayerId;
};

constexpr int ARRAY_SIZE = 5000000;
int MemoryLog(EVENT event, ID sessionID);
void MemLogWriteToFile(int lastIdx);
void MemLogRead(MemLog* memLogBuffer, unsigned long long* pOutCounter, int* pOutLastIdx);
