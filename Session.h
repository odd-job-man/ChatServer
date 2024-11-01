#pragma once
#include "Packet.h"
#include "RingBuffer.h"

class Packet;
struct Session
{
	static constexpr LONG RELEASE_FLAG = 0x80000000;
	SOCKET sock_;
	ULONGLONG id_;
	LONG lSendBufNum_;
	bool bDisconnectCalled_;
	WSAOVERLAPPED recvOverlapped;
	WSAOVERLAPPED sendOverlapped;
	LONG IoCnt_;
	CLockFreeQueue<Packet*> sendPacketQ_;
	BOOL bSendingInProgress_;
	Packet* pSendPacketArr_[50];
	RingBuffer recvRB_;
	BOOL Init(SOCKET clientSock, ULONGLONG ullClientID, SHORT shIdx);
	inline static short GET_SESSION_INDEX(ULONGLONG id)
	{
		return id & 0xFFFF;
	}

};
