#include <WinSock2.h>
#include <windows.h>
#include "CLockFreeQueue.h"
#include "RingBuffer.h"
#include "Session.h"
#include "Player.h"
#include <cstdio>
#include "MemLog.h"
#ifdef MEMLOG
unsigned long long g_Counter;
MemLog g_MemLog[ARRAY_SIZE];

int MemoryLog(EVENT event, ID sessionID)
{
	unsigned long long Cnt = InterlockedIncrement(&g_Counter) - 1;
	int idx = Cnt % ARRAY_SIZE;
	MemLog* pElement = g_MemLog + idx;
	pElement->event = event;
	pElement->Cnt = Cnt;
	pElement->threadId = GetCurrentThreadId();
	pElement->sessionID = sessionID.ullId;
	pElement->PlayerId = Player::MAKE_PLAYER_INDEX(sessionID);
	return idx;
}


void MemLogWriteToFile(int lastIdx)
{
	FILE* pFile;
	_wfopen_s(&pFile, L"MemLog", L"wb");
	if (fwrite((const void*)g_MemLog, sizeof(g_MemLog), 1, pFile) != 1)
		__debugbreak();

	if (fwrite((const void*)&g_Counter, sizeof(g_Counter), 1, pFile) != 1)
		__debugbreak();

	if (fwrite((const void*)&lastIdx, sizeof(int), 1, pFile) != 1)
		__debugbreak();

	fclose(pFile);
}

void MemLogRead(MemLog* memLogBuffer, unsigned long long* pOutCounter, int* pOutLastIdx)
{
	FILE* pFile;
	_wfopen_s(&pFile, L"MemLog", L"rb");
	if (fread_s((void*)memLogBuffer, sizeof(MemLog) * ARRAY_SIZE, sizeof(MemLog) * ARRAY_SIZE, 1, pFile) != 1)
		__debugbreak();

	if (fread_s((void*)pOutCounter, sizeof(unsigned long long), sizeof(unsigned long long), 1, pFile) != 1)
		__debugbreak();

	if (fread_s((void*)pOutLastIdx, sizeof(int), sizeof(int), 1, pFile) != 1)
		__debugbreak();

	fclose(pFile);
}
#endif
