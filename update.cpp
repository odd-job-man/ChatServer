#include <WinSock2.h>
#include "CommonProtocol.h"
#include "Job.h"
#include "CMessageQ.h"
#include "Packet.h"
#include "Player.h"
#include "CSCContents.h"
#include "ErrType.h"
#include "ChatServer.h"

CMessageQ g_MQ;
extern ChatServer g_ChatServer;


bool PacketProc_PACKET(SmartPacket& sp);
bool PacketProc_JOB(SmartPacket& sp);
void Update()
{
	g_MQ.Swap();
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
			PacketProc_JOB(sp);
		++g_ChatServer.UPDATE_CNT_TPS;
	}
	g_ChatServer.SEND_POST_PER_FRAME();
}


bool PacketProc_PACKET(SmartPacket& sp)
{
	try
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
			break;
		case en_PACKET_MONITOR_CLIENT_LOGIN:
		{
			char monitorAccountNo;
			*sp >> monitorAccountNo;
			CS_CHAT_MONITORING_CLIENT_LOGIN(monitorAccountNo, sp->sessionID_);
		}
		break;
		default:
			g_ChatServer.Disconnect(sp->sessionID_);
			break;
		}
	}
	catch (int errCode)
	{
		if (errCode == ERR_PACKET_EXTRACT_FAIL)
		{
			g_ChatServer.Disconnect(sp->sessionID_);
		}
		else if (errCode == ERR_PACKET_RESIZE_FAIL)
		{
			// 지금은 이게 실패할리가 없음 사실 모니터링클라 접속안하면 리사이즈조차 아예 안일어날수도잇음
			LOG(L"RESIZE", ERR, TEXTFILE, L"Resize Fail ShutDown Server");
			__debugbreak();
		}
	}
	return true;
}

bool PacketProc_JOB(SmartPacket& sp)
{
	WORD type;
	*sp >> type;
	switch (type)
	{
	case en_JOB_ON_RELEASE:
		JOB_ON_RELEASE_RECV(sp);
		break;
	default:
		__debugbreak();
		break;
	}
	return true;
}
