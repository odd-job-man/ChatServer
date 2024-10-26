#include <WinSock2.h>
#include <emmintrin.h>
#include "RingBuffer.h"
#include "Packet.h"
#include "Session.h"
#include "Sector.h"
#include "Player.h"

#include "IHandler.h"
#include "NetServer.h"
#include "Logger.h"

extern NetServer g_ChatServer;


std::list<Player*> g_Sector[NUM_OF_SECTOR_VERTICAL][NUM_OF_SECTOR_HORIZONTAL];

void GetSectorAround(WORD sectorX, WORD sectorY, SECTOR_AROUND* pOutSectorAround)
{
	pOutSectorAround->sectorCount = 0;
	__m128i posY = _mm_set1_epi16(sectorY);
	__m128i posX = _mm_set1_epi16(sectorX);

	// 8방향 방향벡터 X성분 Y성분 준비
	__m128i DirVX = _mm_set_epi16(+1, +0, -1, +1, -1, +1, +0, -1);
	__m128i DirVY = _mm_set_epi16(+1, +1, +1, +0, +0, -1, -1, -1);

	// 더한다
	posX = _mm_add_epi16(posX, DirVX);
	posY = _mm_add_epi16(posY, DirVY);

	__m128i min = _mm_set1_epi16(-1);
	__m128i max = _mm_set1_epi16(NUM_OF_SECTOR_VERTICAL);

	// PosX > min ? 0xFFFF : 0
	__m128i cmpMin = _mm_cmpgt_epi16(posX, min);
	// PosX < max ? 0xFFFF : 0
	__m128i cmpMax = _mm_cmplt_epi16(posX, max);
	__m128i resultX = _mm_and_si128(cmpMin, cmpMax);

	SHORT X[8];
	_mm_storeu_si128((__m128i*)X, posX);

	SHORT Y[8];
	cmpMin = _mm_cmpgt_epi16(posY, min);
	cmpMax = _mm_cmplt_epi16(posY, max);
	__m128i resultY = _mm_and_si128(cmpMin, cmpMax);
	_mm_storeu_si128((__m128i*)Y, posY);

	// _mm128i min은 더이상 쓸일이 없으므로 재활용한다.
	min = _mm_and_si128(resultX, resultY);

	SHORT ret[8];
	_mm_storeu_si128((__m128i*)ret, min);

	BYTE sectorCount = 0;
	for (int i = 0; i < 4; ++i)
	{
		if (ret[i] == (short)0xffff)
		{
			pOutSectorAround->Around[sectorCount].sectorY = Y[i];
			pOutSectorAround->Around[sectorCount].sectorX = X[i];
			++sectorCount;
		}
	}

	pOutSectorAround->Around[sectorCount].sectorY = sectorY;
	pOutSectorAround->Around[sectorCount].sectorX = sectorX;
	++sectorCount;

	for (int i = 4; i < 8; ++i)
	{
		if (ret[i] == (short)0xffff)
		{
			pOutSectorAround->Around[sectorCount].sectorY = Y[i];
			pOutSectorAround->Around[sectorCount].sectorX = X[i];
			++sectorCount;
		}
	}
	pOutSectorAround->sectorCount = sectorCount;
}

void RegisterClientAtSector(WORD sectorX, WORD sectorY, Player* pPlayer)
{
	g_Sector[sectorY][sectorX].push_back(pPlayer);
}

void RemoveClientAtSector(WORD sectorX, WORD sectorY, Player* pPlayer)
{
	g_Sector[sectorY][sectorX].remove(pPlayer);
}

void SendPacket_AROUND(SECTOR_AROUND* pSectorAround, SmartPacket& sp)
{
	for (int i = 0; i < pSectorAround->sectorCount; ++i)
	{
		for (auto* player : g_Sector[pSectorAround->Around[i].sectorY][pSectorAround->Around[i].sectorX])
		{
			g_ChatServer.SendPacket(player->sessionId_, sp);
		}
	}
}

void DebugForSectorProb(Player* pPlayer)
{
	auto it = std::find(g_Sector[pPlayer->sectorY_][pPlayer->sectorX_].begin(), g_Sector[pPlayer->sectorY_][pPlayer->sectorX_].end(), pPlayer);
	if (it != g_Sector[pPlayer->sectorY_][pPlayer->sectorX_].end())
		__debugbreak();
}

