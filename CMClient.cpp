#include <WinSock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>
#include "CommonProtocol.h"
#include "CMClient.h"

BOOL CMClient::Start()
{
	for (DWORD i = 0; i < IOCP_ACTIVE_THREAD_NUM_; ++i)
	{
		ResumeThread(hIOCPWorkerThreadArr_[i]);
	}
	InitialConnect();
	return TRUE;
}

void CMClient::OnRecv(ULONGLONG id, Packet* pPacket)
{
}

void CMClient::OnError(ULONGLONG id, int errorType, Packet* pRcvdPacket)
{
}

void CMClient::OnConnect(ULONGLONG id)
{
	LOG(L"ONOFF", SYSTEM, CONSOLE, L"MonitoringServer Connect Success : %u");
	LOG(L"ONOFF", SYSTEM, TEXTFILE, L"MonitoringServer Connect Success : %u");
	SmartPacket sp = PACKET_ALLOC(Lan);
	(*sp) << en_PACKET_SS_MONITOR_LOGIN << SERVERNUM::CHAT;
	SendPacket(id, sp);
}

void CMClient::OnRelease(ULONGLONG id)
{
}

void CMClient::OnConnectFailed(ULONGLONG id)
{
	DWORD errCode = WSAGetLastError();
	LOG(L"ONOFF", ERR, CONSOLE, L"MonitoringServer Connect Failed ErrCode : %u", errCode);
	LOG(L"ONOFF", ERR, TEXTFILE, L"MonitoringServer Connect Failed ErrCode : %u", errCode);
	__debugbreak();
}

void CMClient::SendToMonitoringServer(BYTE serverNo, BYTE dataType, int dataValue, int timeStamp)
{
	SmartPacket sp = PACKET_ALLOC(Lan);
	(*sp) << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE << serverNo << dataType << dataValue << timeStamp;
	SendPacket(pSessionArr_->id_, sp);
}
