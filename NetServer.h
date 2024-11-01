#pragma once
#include "IHandler.h"
#include "CLockFreeStack.h"

class Stack;
class SmartPacket;

class NetServer : public IHandler
{
public:
	virtual BOOL Start();
	virtual void SendPacket(ULONGLONG id, SmartPacket& sendPacket);
	virtual BOOL OnConnectionRequest();
	virtual void* OnAccept(ULONGLONG id);
	virtual void OnRelease(ULONGLONG id);
	virtual void OnRecv(ULONGLONG id, Packet* pPacket);
	virtual void OnError(ULONGLONG id, int errorType, Packet* pRcvdPacket);
	// 디버깅용
	Session* GetSession(ULONGLONG id);
	void Monitoring(int updateCnt, unsigned long long BuffersProcessAtThisFrame);
	void Disconnect(ULONGLONG id);
	void DisconnectAll();
	virtual void Stop();
	static unsigned __stdcall AcceptThread(LPVOID arg);
	static unsigned __stdcall IOCPWorkerThread(LPVOID arg);
private:
	// Accept
	DWORD IOCP_WORKER_THREAD_NUM_;
	LONG lSessionNum_ = 0;
public:
	LONG REQ_MESSAGE_TPS = 0;
	LONG RES_MESSAGE_TPS = 0;
	LONG maxSession_;
	LONG TIME_OUT_MILLISECONDS_;
private:
	ULONGLONG ullIdCounter = 0;
	Session* pSessionArr_;
	CLockFreeStack<short> DisconnectStack_;
	HANDLE hcp_;
	HANDLE hAcceptThread_;
	HANDLE* hIOCPWorkerThreadArr_;
	SOCKET hListenSock_;
	virtual BOOL RecvPost(Session* pSession);
	virtual BOOL SendPost(Session* pSession);
	virtual void ReleaseSession(Session* pSession);
	void RecvProc(Session* pSession, int numberOfBytesTransferred);
	void SendProc(Session* pSession, DWORD dwNumberOfBytesTransferred);
	friend class Packet;

	// Monitoring 변수
	// Recv (Per MSG)

public:
	LONG lPlayerNum = 0;
	LONG lAcceptTotal_PREV = 0;
	alignas(64) LONG lAcceptTotal_ = 0;
	alignas(64) LONG lRecvTPS_ = 0;

	// Send (Per MSG)
	alignas(64) LONG lSendTPS_ = 0;

	// Disconnect
	alignas(64) LONG lDisconnectTPS_ = 0;
};

