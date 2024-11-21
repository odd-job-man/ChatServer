#pragma once
#include "Packet.h"
#include "Player.h"
#include "SCCContents.h"
#include "ChatServer.h"

extern ChatServer g_ChatServer;

void JOB_ON_RELEASE(ULONGLONG sessionID);
void CS_CHAT_REQ_LOGIN(ULONGLONG sessionID, INT64 AccountNo, const WCHAR* pID, const WCHAR* pNickName, const char* pSessionKey);
void CS_CHAT_REQ_SECTOR_MOVE(INT64 accountNo, WORD sectorX, WORD sectorY, ULONGLONG sessionID);
void CS_CHAT_REQ_MESSAGE(INT64 accountNo, WORD messageLen, WCHAR* pMessage, ULONGLONG sessionID);
void CS_CHAT_MONITORING_CLIENT_LOGIN(char MonitoringAccountNo, ULONGLONG sessionID);
void CS_CHAT_SEND_SECTOR_INFO_TO_MONITOR_CLIENT();


__forceinline void JOB_ON_RELEASE_RECV(SmartPacket& sp)
{
	JOB_ON_RELEASE(sp->sessionID_);
}

__forceinline void CS_CHAT_REQ_LOGIN_RECV(SmartPacket& sp)
{
	INT64 AccountNo;
	*sp >> AccountNo;

	WCHAR* pID = (WCHAR*)sp->GetPointer(sizeof(WCHAR) * Player::ID_LEN);
	WCHAR* pNickName = (WCHAR*)sp->GetPointer(sizeof(WCHAR) * Player::NICK_NAME_LEN);
	char* pSessionKey = (char*)sp->GetPointer(sizeof(char) * Player::SESSION_KEY_LEN);

	if (sp->IsBufferEmpty() == false)
	{
		g_ChatServer.Disconnect(sp->sessionID_);
		return;
	}

	CS_CHAT_REQ_LOGIN(sp->sessionID_, AccountNo, pID, pNickName, pSessionKey);
}

__forceinline void CS_CHAT_REQ_SECTOR_MOVE_RECV(SmartPacket& sp)
{
	INT64 accountNo;
	WORD sectorX;
	WORD sectorY;
	*sp >> accountNo >> sectorX >> sectorY;
	if (sp->IsBufferEmpty() == false)
	{
		g_ChatServer.Disconnect(sp->sessionID_);
		return;
	}
	CS_CHAT_REQ_SECTOR_MOVE(accountNo, sectorX, sectorY, sp->sessionID_);
}

__forceinline void CS_CHAT_REQ_MESSAGE_RECV(SmartPacket& sp)
{
	INT64 accountNo;
	*sp >> accountNo;
	WORD messageLen;
	*sp >> messageLen;
	WCHAR* pMessage = (WCHAR*)sp->GetPointer(messageLen);

	if (pMessage == nullptr || sp->IsBufferEmpty() == false)
	{
		g_ChatServer.Disconnect(sp->sessionID_);
		return;
	}
	CS_CHAT_REQ_MESSAGE(accountNo, messageLen, pMessage, sp->sessionID_);
}



