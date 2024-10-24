#include <WinSock2.h>
#include <time.h>
#include"Packet.h"
#include "Session.h"
#include "IHandler.h"
#include "NetServer.h"
#include "CMessageQ.h"
#include "Packet.h"
#include <crtdbg.h>

#pragma comment(lib,"Winmm.lib")

int g_iOldFrameTick;
int g_iFpsCheck;
int g_iTime;
int g_iFPS;
int g_iNetworkLoop;
int g_iFirst;
constexpr int TICK_PER_FRAME = 40;
constexpr int FRAME_PER_SECONDS = (1000) / TICK_PER_FRAME;

extern NetServer g_ChatServer;

bool Update();

int main()
{
	timeBeginPeriod(1);
	srand((unsigned)time(nullptr));
	g_ChatServer.Start(5000);

	g_iFirst = timeGetTime();
	g_iOldFrameTick = g_iFirst;
	g_iTime = g_iOldFrameTick;
	g_iFPS = 0;
	g_iFpsCheck = g_iOldFrameTick;

	while (true)
	{
		if (!Update())
			break;
		g_iTime = timeGetTime();
		// 프레임 밀렷을때 
		if (g_iTime - g_iOldFrameTick >= TICK_PER_FRAME)
		{
			g_iOldFrameTick = g_iTime - ((g_iTime - g_iFirst) % TICK_PER_FRAME);
			continue;
		}
		Sleep(TICK_PER_FRAME - (g_iTime - g_iOldFrameTick));
		g_iOldFrameTick += TICK_PER_FRAME;
	}

	while (true)
	{
		Sleep(1);
	}

	return 0;
}