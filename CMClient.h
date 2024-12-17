#pragma once
#include "LanClient.h"

class CMClient : public LanClient
{
public:
	BOOL Start();
	virtual void OnRecv(ULONGLONG id, Packet* pPacket) override;
	virtual void OnError(ULONGLONG id, int errorType, Packet* pRcvdPacket) override;
	virtual void OnConnect(ULONGLONG id) override;
	virtual void OnRelease(ULONGLONG id) override;
	virtual void OnConnectFailed(ULONGLONG id) override;
	void SendToMonitoringServer(BYTE serverNo, BYTE DataType, int dataValue, int timeStamp);
};
