#pragma once
#include "NetServer.h"
#include "CMClient.h"
#include "HMonitor.h"

class ChatServer : public NetServer
{
public:
	ChatServer();
	void Start();
	virtual BOOL OnConnectionRequest();
	virtual void* OnAccept(ULONGLONG id);
	virtual void OnRelease(ULONGLONG id);
	virtual void OnRecv(ULONGLONG id, Packet* pPacket);
	virtual void OnError(ULONGLONG id, int errorType, Packet* pRcvdPacket);
	virtual void OnPost(void* order);
	void Monitoring();
	void DisconnectAll();
	const LONG TICK_PER_FRAME_;
	const ULONGLONG SESSION_TIMEOUT_;
	const ULONGLONG PLAYER_TIMEOUT_;
	LONG UPDATE_CNT_TPS = 0;
	ULONGLONG UPDATE_CNT_TOTAL = 0;
	ULONGLONG RECV_TOTAL = 0;
	ULONGLONG PROCESS_CPU_TICK_ELAPSED = 0;
	ULONGLONG PROCESS_CPU_TICK_TIME_DIFF = 0;
	static inline HMonitor monitor;
	CMClient* pLanClient_;
};
