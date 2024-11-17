#pragma once
enum class OVERLAPPED_REASON
{
	SEND,
	RECV,
	TIMEOUT,
	SEND_POST_FRAME,
};

struct MYOVERLAPPED
{
	WSAOVERLAPPED overlapped;
	OVERLAPPED_REASON why;
};

