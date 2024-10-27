#include <WinSock2.h>
#include "CSCContents.h"
#include "Sector.h"
#include "MemLog.h"
NetServer g_ChatServer;

void JOB_ON_ACCEPT(WORD playerIdx, ID sessionId)
{
	// 이미 true라는 이야기는 이중 ON_ACCEPT임
	Player* pPlayer = Player::pPlayerArr + playerIdx;
	if (pPlayer->bUsing_ == true)
	{
		MEMORY_LOG_WRITE_TO_FILE(MEMORY_LOG(J_ON_ACCEPT,pPlayer->sessionId_));
		__debugbreak();
	}
	pPlayer->LastRecvedTime_ = GetTickCount64();
	pPlayer->bUsing_ = true;
	pPlayer->sessionId_ = sessionId;
}

void JOB_ON_RELEASE(WORD playerIdx)
{
	// 이미 RELEASE 된 플레이어에 대해서 다시한번 RELEASE JOB이 도착한것임
	Player* pPlayer = Player::pPlayerArr + playerIdx;
	if (pPlayer->bUsing_ == false)
	{
		MEMORY_LOG_WRITE_TO_FILE(MEMORY_LOG(J_ON_RELEASE,pPlayer->sessionId_));
		__debugbreak();
	}

	if (pPlayer->sectorX_ != Player::INITIAL_SECTOR_VALUE && pPlayer->sectorY_ != Player::INITIAL_SECTOR_VALUE)
		RemoveClientAtSector(pPlayer->sectorX_, pPlayer->sectorY_, pPlayer);

	pPlayer->bUsing_ = false;
	--g_ChatServer.lPlayerNum;
}

void CS_CHAT_REQ_LOGIN(WORD playerIdx, INT64 AccountNo, const WCHAR* pID, const WCHAR* pNickName, const char* pSessionKey)
{
	Player* pPlayer = Player::pPlayerArr + playerIdx;
	pPlayer->LastRecvedTime_ = GetTickCount64();
	pPlayer->accountNo_ = AccountNo;
	pPlayer->sectorX_ = pPlayer->sectorY_ = Player::INITIAL_SECTOR_VALUE;

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

	// 클라가 유효하지 않은 좌표로 요청햇다면 끊어버린다
	if (isNonValidSector(sectorX, sectorY))
	{
		g_ChatServer.Disconnect(pPlayer->sessionId_);
		return;
	}

	if (pPlayer->sectorX_ != Player::INITIAL_SECTOR_VALUE && pPlayer->sectorY_ != Player::INITIAL_SECTOR_VALUE)
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

	// 디버깅용
	if (pPlayer->accountNo_ != accountNo)
	{
		g_ChatServer.Disconnect(pPlayer->sessionId_);
		return;
	}

	SmartPacket sp = PACKET_ALLOC(Net);
	sp->WRITE_PACKET_LOG(Packet::ALLOC_CS_CHAT_REQ_MESSAGE, sp->refCnt_, g_ChatServer.GetSession(pPlayer->sessionId_), pPlayer->sessionId_);
	MAKE_CS_CHAT_RES_MESSAGE(accountNo, pPlayer->ID_, pPlayer->nickName_, messageLen, pMessage, sp);
	SECTOR_AROUND sectorAround;
	GetSectorAround(pPlayer->sectorX_, pPlayer->sectorY_, &sectorAround);
	SendPacket_AROUND(&sectorAround, sp);
}

void CS_CHAT_REQ_HEARTBEAT(WORD playerIdx)
{
	Player::pPlayerArr[playerIdx].LastRecvedTime_ = GetTickCount64();
}