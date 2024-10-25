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
extern Player g_playerArr[5000];
extern NetServer g_ChatServer;

constexpr int TIME_OUT_MILLISECONDS = 4000 * 10;


bool PacketProc_PACKET(SmartPacket& sp);
bool PacketProc_JOB(SmartPacket& sp);

unsigned long long Update()
{
	g_MQ.Swap();
	unsigned long long ret = g_MQ.BuffersToProcessThisFrame_;
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
	}


	for (int i = 0; i < Player::MAX_PLAYER_NUM; ++i)
	{
		Player* pPlayer = g_playerArr + i;
		if (pPlayer->bUsing_ == false)
			continue;

		//40초 지나면 타임 아웃 처리
		if (GetTickCount64() - pPlayer->LastRecvedTime_ > TIME_OUT_MILLISECONDS)
			g_ChatServer.Disconnect(pPlayer->sessionId_);
	}
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
		g_ChatServer.OnError(g_playerArr[sp->playerIdx_].sessionId_, (int)PACKET_PROC_RECVED_PACKET_INVALID_TYPE, sp.GetPacket());
		g_ChatServer.Disconnect(g_playerArr[sp->playerIdx_].sessionId_);
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
