#include "WinSock2.h"
#include "Packet.h"
#include "NetSession.h"
#include "Player.h"
#include "SCCContents.h"

void MAKE_CS_CHAT_RES_LOGIN(WORD type, BYTE status, INT64 accountNo, SmartPacket& sp)
{
	*sp << type << status << accountNo;
}

void MAKE_CS_CHAT_RES_SECTOR_MOVE(INT64 accountNo, WORD sectorX, WORD sectorY, SmartPacket& sp)
{
	*sp << (WORD)en_PACKET_CS_CHAT_RES_SECTOR_MOVE << accountNo << sectorX << sectorY;
}

void MAKE_CS_CHAT_RES_MESSAGE(INT64 accountNo, WCHAR* pID, WCHAR* pNickName, WORD messageLen, WCHAR* pMessage, SmartPacket& sp)
{
	*sp << (WORD)en_PACKET_CS_CHAT_RES_MESSAGE << accountNo;
	if (sp->PutData((char*)pID, Player::ID_LEN * 2) == 0)
		goto lb_fail;
	if(sp->PutData((char*)pNickName, Player::NICK_NAME_LEN * 2) == 0)
		goto lb_fail;

	*sp << messageLen;

	if (sp->PutData((char*)pMessage, messageLen) == 0)
		goto lb_fail;

	return;

lb_fail:
	LOG(L"RESIZE", ERR, TEXTFILE, L"Resize Fail ShutDown Server");
	__debugbreak();
}
