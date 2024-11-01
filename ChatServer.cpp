#include <WinSock2.h>
#include <stdio.h>
#include "Windows.h"
#include "ChatServer.h"
#include "Packet.h"
#include "Player.h"
#include "Job.h"
#include "CMessageQ.h"
#include "ErrType.h"
#include "Logger.h"

#include "Parser.h"

extern CMessageQ g_MQ;

ChatServer g_ChatServer;

ChatServer::ChatServer()
{
	char* pStart;
	PARSER psr = CreateParser(L"ServerConfig.txt");

	GetValue(psr, L"USER_MAX", (PVOID*)&pStart, nullptr);
	Player::MAX_PLAYER_NUM = (short)_wtoi((LPCWSTR)pStart);
	ReleaseParser(psr);

	Player::pPlayerArr = new Player[maxSession_];
}

BOOL ChatServer::OnConnectionRequest()
{
    if (lSessionNum_ + 1 >= maxSession_)
        return FALSE;

    return TRUE;
}

void* ChatServer::OnAccept(ULONGLONG id)
{
	Packet* pPacket = PACKET_ALLOC(Net);
	pPacket->playerIdx_ = Player::MAKE_PLAYER_INDEX(id);
	pPacket->recvType_ = JOB;
	*pPacket << (WORD)en_JOB_ON_ACCEPT << id;
	g_MQ.Enqueue(pPacket);
	return nullptr;
}

void ChatServer::OnRelease(ULONGLONG id)
{
	Packet* pPacket = PACKET_ALLOC(Net);
	pPacket->playerIdx_ = Session::GET_SESSION_INDEX(id);
	pPacket->recvType_ = JOB;
	*pPacket << (WORD)en_JOB_ON_RELEASE;
	g_MQ.Enqueue(pPacket);
}

void ChatServer::OnRecv(ULONGLONG id, Packet* pPacket)
{
	pPacket->playerIdx_ = Player::MAKE_PLAYER_INDEX(id);
	pPacket->recvType_ = RECVED_PACKET;
	g_MQ.Enqueue(pPacket);
}

void ChatServer::OnError(ULONGLONG id, int errorType, Packet* pRcvdPacket)
{
	switch ((ErrType)errorType)
	{
	case PACKET_PROC_RECVED_PACKET_INVALID_TYPE:
	{
		pRcvdPacket->MoveReadPos(-(int)sizeof(WORD));
		WORD type;
		*pRcvdPacket >> type;
		LOG(L"PACKET_PROC_RECVED_PACKET_INVALID_TYPE", SYSTEM, TEXTFILE, L"Session ID : %d Recved Type : %x", id, type);
		break;
	}
	default:
		__debugbreak();
		break;
	}
}

void ChatServer::Monitoring(int updateCnt, unsigned long long BuffersProcessAtThisFrame)
{
	printf(
		"update Count : %llu\n"
		"Packet Pool Alloc Capacity : %d\n"
		"MessageQ Capacity : %d\n"
		"MessageQ Queued By Worker : %llu\n"
		"SendQ Pool Capacity : %d\n"
		"Accept TPS: %d\n"
		"Accept Total : %d\n"
		"Disconnect TPS: %d\n"
		"Recv Msg TPS: %d\n"
		"Send Msg TPS: %d\n"
		"Session Num : %d\n"
		"Player Num : %d\n"
		"REQ_MESSAGE_TPS : %d\n"
		"RES_MESSAGE_TPS : %d\n\n",
		BuffersProcessAtThisFrame,
		Packet::packetPool_.capacity_,
		g_MQ.packetPool_.capacity_,
		g_MQ.workerEnqueuedBufferCnt_,
		pSessionArr_[0].sendPacketQ_.nodePool_.capacity_,
		lAcceptTotal_ - lAcceptTotal_PREV,
		lAcceptTotal_,
		lDisconnectTPS_,
		lRecvTPS_,
		lSendTPS_,
		lSessionNum_,
		lPlayerNum,
		REQ_MESSAGE_TPS,
		RES_MESSAGE_TPS);

	lAcceptTotal_PREV = lAcceptTotal_;
	InterlockedExchange(&lDisconnectTPS_, 0);
	InterlockedExchange(&lRecvTPS_, 0);
	InterlockedExchange(&lSendTPS_, 0);
	REQ_MESSAGE_TPS = 0;
	RES_MESSAGE_TPS = 0;
}

void ChatServer::DisconnectAll()
{
	closesocket(hListenSock_);
	WaitForSingleObject(hAcceptThread_, INFINITE);
	CloseHandle(hAcceptThread_);

	LONG MaxSession = maxSession_;
	for (LONG i = 0; i < MaxSession; ++i)
	{
		// RELEASE 중이 아니라면 Disconnect
		pSessionArr_[i].bDisconnectCalled_ = true;
		CancelIoEx((HANDLE)pSessionArr_[i].sock_, nullptr);
	}

	while (lSessionNum_ != 0);
}
