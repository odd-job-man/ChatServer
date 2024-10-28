#pragma once
#include<windows.h>
#include "CLinkedList.h"
struct Player
{
	static constexpr int ID_LEN = 20;
	static constexpr int NICK_NAME_LEN = 20;
	static constexpr int SESSION_KEY_LEN = 64;
	static inline int MAX_PLAYER_NUM;
	static inline Player* pPlayerArr;
	static constexpr int INITIAL_SECTOR_VALUE = 51;

	bool bUsing_;
	bool bLogin_;
	bool bInitialMove_;
	LINKED_NODE sectorLink;
	ID sessionId_;
	WORD sectorX_;
	WORD sectorY_;
	INT64 accountNo_;
	ULONGLONG LastRecvedTime_;
	WCHAR ID_[ID_LEN];
	WCHAR nickName_[NICK_NAME_LEN];
	// 좀 애매함

	Player()
		:sectorLink{ offsetof(Player,sectorLink) }, bUsing_{ false }, bLogin_{ false }, bInitialMove_{ false }
	{}

	static WORD MAKE_PLAYER_INDEX(ID sessionId)
	{
		return sessionId.ullId & 0xFFFF;
	}
};
