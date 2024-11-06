#pragma once
#include <NetServer.h>

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
	virtual void OnPost(int order);
	void Monitoring(int updateCnt, unsigned long long BuffersProcessAtThisFrame);
	void DisconnectAll();
	LONG REQ_MESSAGE_TPS = 0;
	LONG RES_MESSAGE_TPS = 0;
};
