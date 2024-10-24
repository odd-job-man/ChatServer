#pragma once
#include<windows.h>
struct Player
{
	static constexpr int ID_LEN = 20;
	static constexpr int NICK_NAME_LEN = 20;
	static constexpr int SESSION_KEY_LEN = 64;
	static constexpr int MAX_PLAYER_NUM = 15000;
	static constexpr int INITIAL_SECTOR_VALUE = 51;

	bool bUsing_ = false;
	ID sessionId_;
	WORD sectorX_;
	WORD sectorY_;
	INT64 accountNo_;
	ULONGLONG LastRecvedTime_;
	WCHAR ID_[ID_LEN];
	WCHAR nickName_[NICK_NAME_LEN];
	// 좀 애매함
	//char sessionKey_[SESSION_KEY_LEN];
	static WORD MAKE_PLAYER_INDEX(ID sessionId)
	{
		return sessionId.ullId & 0xFFFF;
	}
};
