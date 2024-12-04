#include <WinSock2.h>
#include "CSCContents.h"
#include "Sector.h"
#include "MemLog.h"
#include "ChatServer.h"

extern ChatServer g_ChatServer;

bool g_bMonitoringClientLogin = false;
ULONGLONG g_monitoringClientSessionID = ULLONG_MAX;

void JOB_ON_RELEASE(ULONGLONG sessionID)
{
	// �̹� RELEASE �� �÷��̾ ���ؼ� �ٽ��ѹ� RELEASE JOB�� �����Ѱ���
	Player* pPlayer = Player::pPlayerArr + Player::MAKE_PLAYER_INDEX(sessionID);

	// ���Ǹ� �����ǰ� �α��� ���ϴٰ� ���Ƿ� ���ų� Ÿ�Ӿƿ��Ȱ��
	if (pPlayer->bLogin_ == false)
		return;
	else
		pPlayer->bLogin_ = false;


	if (g_bMonitoringClientLogin == true)
	{
		if(g_monitoringClientSessionID == pPlayer->sessionId_)
			g_bMonitoringClientLogin = false;
	}

	// �α����޴ٸ� �̹� �̵� �޽����� ������ ��ϵ� ��ǥ�� �ش��ϴ� ���Ϳ� ����������̹Ƿ� �����Ѵ�.
	if (pPlayer->bRegisterAtSector_ == true)
		RemoveClientAtSector(pPlayer->sectorX_, pPlayer->sectorY_, pPlayer);
	--g_ChatServer.lPlayerNum;
}

void CS_CHAT_REQ_LOGIN(ULONGLONG sessionID, INT64 AccountNo, const WCHAR* pID, const WCHAR* pNickName, const char* pSessionKey)
{
	// �÷��̾���� ������ �Ѿ�� �α��� ���н�Ŵ
	Player* pPlayer = Player::pPlayerArr + Player::MAKE_PLAYER_INDEX(sessionID);
	if (g_ChatServer.lPlayerNum >= Player::MAX_PLAYER_NUM)
	{
		g_ChatServer.Disconnect(pPlayer->sessionId_);
		return;
	}
	pPlayer->bLogin_ = true;
	pPlayer->bRegisterAtSector_ = false;
	pPlayer->accountNo_ = AccountNo;
	pPlayer->sessionId_ = sessionID;

	wcscpy_s(pPlayer->ID_, Player::ID_LEN, pID);
	wcscpy_s(pPlayer->nickName_, Player::NICK_NAME_LEN, pNickName);

	SmartPacket sp = PACKET_ALLOC(Net);
	MAKE_CS_CHAT_RES_LOGIN(en_PACKET_CS_CHAT_RES_LOGIN, 1, AccountNo, sp);
	g_ChatServer.SENDPACKET(pPlayer->sessionId_, sp);
	//g_ChatServer.SendPacket(pPlayer->sessionId_, sp.GetPacket());
	//g_ChatServer.SendPacket_ENQUEUE_ONLY(pPlayer->sessionId_, sp.GetPacket());
	++g_ChatServer.lPlayerNum;
}

void CS_CHAT_REQ_SECTOR_MOVE(INT64 accountNo, WORD sectorX, WORD sectorY, ULONGLONG sessionID)
{
	Player* pPlayer = Player::pPlayerArr + Player::MAKE_PLAYER_INDEX(sessionID);

	if (pPlayer->bLogin_ == false)
	{
		g_ChatServer.Disconnect(pPlayer->sessionId_);
		return;
	}

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
	g_ChatServer.SENDPACKET(pPlayer->sessionId_, sp);
	//g_ChatServer.SendPacket_ENQUEUE_ONLY(pPlayer->sessionId_, sp.GetPacket());
	//g_ChatServer.SendPacket(pPlayer->sessionId_, sp.GetPacket());
}

void CS_CHAT_REQ_MESSAGE(INT64 accountNo, WORD messageLen, WCHAR* pMessage, ULONGLONG sessionID)
{
	Player* pPlayer = Player::pPlayerArr + Player::MAKE_PLAYER_INDEX(sessionID);

	if (pPlayer->bLogin_ == false)
	{
		g_ChatServer.Disconnect(pPlayer->sessionId_);
		return;
	}

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
	SendPacket_AROUND(&sectorAround, sp);
}


constexpr char MONITOR_CLINET_ACCOUNT_NO = 1;

void CS_CHAT_MONITORING_CLIENT_LOGIN(char MonitoringAccountNo, ULONGLONG sessionID)
{
	Player* pPlayer = Player::pPlayerArr + Player::MAKE_PLAYER_INDEX(sessionID);
	pPlayer->sessionId_ = sessionID;
	pPlayer->accountNo_ = MonitoringAccountNo;
	pPlayer->bLogin_ = true;
	++g_ChatServer.lPlayerNum;

	if (MonitoringAccountNo == MONITOR_CLINET_ACCOUNT_NO)
	{
		if (g_bMonitoringClientLogin == true)
			g_ChatServer.Disconnect(g_monitoringClientSessionID);
		else
			g_bMonitoringClientLogin = true;

		g_monitoringClientSessionID = sessionID;
	}
	else
	{
		g_ChatServer.Disconnect(sessionID);
		return;
	}
}

class MonitorPacket
{
public:
	Packet packet_;

	MonitorPacket()
	{
		packet_.IncreaseRefCnt();
	}


	~MonitorPacket()
	{
		packet_.DecrementRefCnt();
		delete[] packet_.pBuffer_;
	}

	__forceinline Packet* GetPacket()
	{
		packet_.Clear<Net>();
		return &packet_;
	}

	__forceinline bool canSend()
	{
		if (packet_.refCnt_ > 1)
			return false;

		if (packet_.refCnt_ <= 0)
			__debugbreak();

		return true;
	}

	__forceinline void SendPacket()
	{
		if(g_ChatServer.bAccSend == 1)
			g_ChatServer.SendPacket_ENQUEUE_ONLY(g_monitoringClientSessionID, &packet_);
		else
			g_ChatServer.SendPacket(g_monitoringClientSessionID, &packet_);
	}

}g_MonitorPacket;

Packet* g_monitoringPacket = nullptr;

void CS_CHAT_SEND_SECTOR_INFO_TO_MONITOR_CLIENT()
{
	static ULONGLONG FirstTimeCheckForMonitor = GetTickCount64();
	static ULONGLONG timeCheckForMonitor = FirstTimeCheckForMonitor;

	ULONGLONG currentTime = GetTickCount64();
	if (currentTime < timeCheckForMonitor + 1000)
		return;

	if (g_bMonitoringClientLogin == false)
		return;

	if (g_MonitorPacket.canSend() == false)
		return;

	GetSectorMonitoringInfo(g_MonitorPacket.GetPacket());
	g_MonitorPacket.SendPacket();

	timeCheckForMonitor = currentTime - ((currentTime - FirstTimeCheckForMonitor) % 1000);
}