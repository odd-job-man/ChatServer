#include <WinSock2.h>
#include "CommonProtocol.h"
#include "Job.h"
#include "CMessageQ.h"
#include "Packet.h"
#include "Player.h"
#include "SCCContents.h"
#include "CSCContents.h"
#include "Logger.h"
#include "ErrType.h"
extern CMessageQ g_MQ;
extern NetServer g_ChatServer;

bool PacketProc_PACKET(SmartPacket& sp);
bool PacketProc_JOB(SmartPacket& sp);

unsigned long long Update()
{
	static unsigned long long timeOutCheck = GetTickCount64();
	g_MQ.Swap();
	unsigned long long ret = 0;
	bool bStop = false;
	while (true)
	{
		SmartPacket sp = g_MQ.Dequeue();
		if (sp.GetPacket() == nullptr)
			break;

		if (sp->recvType_ == RECVED_PACKET)
		{
			PacketProc_PACKET(sp);
		}
		else
		{
			PacketProc_JOB(sp);
		}

		if (GetAsyncKeyState(VK_RETURN) & 0x01)
		{
			g_ChatServer.DisconnectAll();
			bStop = true;
			break;
		}
		++ret;
	}

	if (bStop)
	{
		// 메시지 큐 비우기
		Packet* pPacket;
		while ((pPacket = g_MQ.Dequeue()) != nullptr)
		{
			Packet::Free(pPacket);
		}
		g_MQ.Swap();
		while ((pPacket = g_MQ.Dequeue()) != nullptr)
		{
			Packet::Free(pPacket);
		}
		g_ChatServer.Stop();
		return -1;
	}

	unsigned long long currentTime = GetTickCount64();
	// 3초에 한번씩 타임아웃 체크함(우선 하드코딩)
	if (currentTime - timeOutCheck <= 30000)
		return ret;

	for (int i = 0; i < g_ChatServer.maxSession_; ++i)
	{
		Player* pPlayer = Player::pPlayerArr + i;
		if (pPlayer->bUsing_ == false)
			continue;

		//40초 지나면 타임 아웃 처리
		if (currentTime - pPlayer->LastRecvedTime_ > g_ChatServer.TIME_OUT_MILLISECONDS_)
			g_ChatServer.Disconnect(pPlayer->sessionId_);
	}

	timeOutCheck += 30000;
	return ret;
}

bool PacketProc_PACKET(SmartPacket& sp)
{
	WORD type;
	*sp >> type;
	switch (type)
	{
	case en_PACKET_CS_CHAT_REQ_LOGIN:
		CS_CHAT_REQ_LOGIN_RECV(sp);
		break;
	case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
		CS_CHAT_REQ_SECTOR_MOVE_RECV(sp);
		break;
	case en_PACKET_CS_CHAT_REQ_MESSAGE:
		CS_CHAT_REQ_MESSAGE_RECV(sp);
		break;
	case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
		CS_CHAT_REQ_HEARTBEAT_RECV(sp);
		break;
	default:
		g_ChatServer.OnError(Player::pPlayerArr[sp->playerIdx_].sessionId_, (int)PACKET_PROC_RECVED_PACKET_INVALID_TYPE, sp.GetPacket());
		g_ChatServer.Disconnect(Player::pPlayerArr[sp->playerIdx_].sessionId_);
		break;
	}
	return true;
}

bool PacketProc_JOB(SmartPacket& sp)
{
	WORD type;
	*sp >> type;
	switch (type)
	{
	case en_JOB_ON_ACCEPT:
		JOB_ON_ACCEPT_RECV(sp);
		break;
	case en_JOB_ON_RELEASE:
		JOB_ON_RELEASE_RECV(sp);
		break;
	default:
		__debugbreak();
		break;
	}
	return true;
}
