#include <WinSock2.h>
#include <time.h>
#include <stdio.h>

#include "ChatServer.h"

#pragma comment(lib,"Winmm.lib")

int g_iOldFrameTick;
int g_iFpsCheck;
int g_iTime;
int g_iFPS;
int g_iFirst;

extern ChatServer g_ChatServer;

void Update();
void CS_CHAT_SEND_SECTOR_INFO_TO_MONITOR_CLIENT();

int main()
{
	timeBeginPeriod(1);
	srand((unsigned)time(nullptr));

	g_iFirst = timeGetTime();
	g_iOldFrameTick = g_iFirst;
	g_iTime = g_iOldFrameTick;
	g_iFPS = 0;
	g_iFpsCheck = g_iOldFrameTick;

	unsigned long long sum = 0;
	g_ChatServer.Start();
	while (true)
	{
		Update();
		g_iTime = timeGetTime();
		++g_iFPS;
		CS_CHAT_SEND_SECTOR_INFO_TO_MONITOR_CLIENT();
		// 프레임 밀렷을때 
		if (g_iTime - g_iFpsCheck >= 1000)
		{
			printf("FPS : %d\n", g_iFPS);
			g_ChatServer.Monitoring();
			g_iFPS = 0;
			g_iFpsCheck += 1000;
			sum = 0;
		}

		if (g_iTime - g_iOldFrameTick >= g_ChatServer.TICK_PER_FRAME_)
		{
			g_iOldFrameTick = g_iTime - ((g_iTime - g_iFirst) % g_ChatServer.TICK_PER_FRAME_);
			continue;
		}

		Sleep(g_ChatServer.TICK_PER_FRAME_ - (g_iTime - g_iOldFrameTick));
		g_iOldFrameTick += g_ChatServer.TICK_PER_FRAME_;
	}

	while (true)
	{
		Sleep(1);
	}

	return 0;
}