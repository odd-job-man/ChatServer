#include <WinSock2.h>
#include "CSCContents.h"
#include "Sector.h"
#include "MemLog.h"
#include "ChatServer.h"

extern ChatServer g_ChatServer;

void JOB_ON_ACCEPT(WORD playerIdx, ULONGLONG sessionId)
{
	// �̹� true��� �̾߱�� ���� ON_ACCEPT��
	Player* pPlayer = Player::pPlayerArr + playerIdx;
	if (pPlayer->bUsing_ == true)
	{
		MEMORY_LOG_WRITE_TO_FILE(MEMORY_LOG(J_ON_ACCEPT,pPlayer->sessionId_));
		__debugbreak();
	}
	pPlayer->bUsing_ = true;
	pPlayer->bLogin_ = false;
	pPlayer->bRegisterAtSector_ = false;
	pPlayer->sessionId_ = sessionId;
	pPlayer->LastRecvedTime_ = GetTickCount64();
}

void JOB_ON_RELEASE(WORD playerIdx)
{
	// �̹� RELEASE �� �÷��̾ ���ؼ� �ٽ��ѹ� RELEASE JOB�� �����Ѱ���
	Player* pPlayer = Player::pPlayerArr + playerIdx;
	if (pPlayer->bUsing_ == false)
	{
		MEMORY_LOG_WRITE_TO_FILE(MEMORY_LOG(J_ON_RELEASE,pPlayer->sessionId_));
		__debugbreak();
	}

	pPlayer->bUsing_ = false;
	// �α����޴ٸ� �̹� �̵� �޽����� ������ ��ϵ� ��ǥ�� �ش��ϴ� ���Ϳ� ����������̹Ƿ� �����Ѵ�.
	if (pPlayer->bLogin_ == true)
	{
		if (pPlayer->bRegisterAtSector_ == true)
			RemoveClientAtSector(pPlayer->sectorX_, pPlayer->sectorY_, pPlayer);
		--g_ChatServer.lPlayerNum;
	}
}

void CS_CHAT_REQ_LOGIN(WORD playerIdx, INT64 AccountNo, const WCHAR* pID, const WCHAR* pNickName, const char* pSessionKey)
{
	// �÷��̾���� ������ �Ѿ�� �α��� ���н�Ŵ
	Player* pPlayer = Player::pPlayerArr + playerIdx;
	if (g_ChatServer.lPlayerNum >= Player::MAX_PLAYER_NUM)
	{
		g_ChatServer.Disconnect(pPlayer->sessionId_);
		return;
	}
	pPlayer->LastRecvedTime_ = GetTickCount64();
	pPlayer->accountNo_ = AccountNo;
	pPlayer->bLogin_ = true;

	wcscpy_s(pPlayer->ID_, Player::ID_LEN, pID);
	wcscpy_s(pPlayer->nickName_, Player::NICK_NAME_LEN, pNickName);

	SmartPacket sp = PACKET_ALLOC(Net);
	MAKE_CS_CHAT_RES_LOGIN(en_PACKET_CS_CHAT_RES_LOGIN, 1, AccountNo, sp);
	g_ChatServer.SendPacket(pPlayer->sessionId_, sp);
	++g_ChatServer.lPlayerNum;
}

void CS_CHAT_REQ_SECTOR_MOVE(INT64 accountNo, WORD sectorX, WORD sectorY, WORD playerIdx)
{
	Player* pPlayer = Player::pPlayerArr + playerIdx;
	pPlayer->LastRecvedTime_ = GetTickCount64();

	if (pPlayer->accountNo_ != accountNo)
	{
		g_ChatServer.Disconnect(pPlayer->sessionId_);
		return;
	}
	
	// Ŭ�� ��ȿ���� ���� ��ǥ�� ��û�޴ٸ� ���������
	if (IsNonValidSector(sectorX, sectorY))
	{
		g_ChatServer.Disconnect(pPlayer->sessionId_);
		return;
	}

	if (pPlayer->bRegisterAtSector_ == false)
		pPlayer->bRegisterAtSector_ = true;
	else
		RemoveClientAtSector(pPlayer->sectorX_, pPlayer->sectorY_, pPlayer);

	pPlayer->sectorX_ = sectorX;
	pPlayer->sectorY_ = sectorY;
	RegisterClientAtSector(sectorX, sectorY, pPlayer);

	SmartPacket sp = PACKET_ALLOC(Net);
	MAKE_CS_CHAT_RES_SECTOR_MOVE(accountNo, sectorX, sectorY, sp);
	g_ChatServer.SendPacket(pPlayer->sessionId_, sp);
}

void CS_CHAT_REQ_MESSAGE(INT64 accountNo, WORD messageLen, WCHAR* pMessage, WORD playerIdx)
{
	Player* pPlayer = Player::pPlayerArr + playerIdx;
	pPlayer->LastRecvedTime_ = GetTickCount64();

	// ������
	if (pPlayer->accountNo_ != accountNo)
	{
		g_ChatServer.Disconnect(pPlayer->sessionId_);
		return;
	}

	SmartPacket sp = PACKET_ALLOC(Net);
	MAKE_CS_CHAT_RES_MESSAGE(accountNo, pPlayer->ID_, pPlayer->nickName_, messageLen, pMessage, sp);
	SECTOR_AROUND sectorAround;
	GetSectorAround(pPlayer->sectorX_, pPlayer->sectorY_, &sectorAround);
	++g_ChatServer.REQ_MESSAGE_TPS;
	SendPacket_AROUND(&sectorAround, sp);
}

void CS_CHAT_REQ_HEARTBEAT(WORD playerIdx)
{
	Player::pPlayerArr[playerIdx].LastRecvedTime_ = GetTickCount64();
}