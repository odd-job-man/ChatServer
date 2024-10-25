#pragma once
#include "CLockFreeStack.h"

class Stack;
class SmartPacket;

class NetServer : public IHandler
{
public:
	virtual BOOL Start(DWORD dwMaxSession);
	virtual void SendPacket(ID id, SmartPacket& sendPacket);
	virtual BOOL OnConnectionRequest();
	virtual void* OnAccept(ID id);
	virtual void OnRelease(ID id);
	virtual void OnRecv(ID id, Packet* pPacket);
	virtual void OnError(ID id, int errorType, Packet* pRcvdPacket);
	void Monitoring(int updateCnt, unsigned long long BuffersProcessAtThisFrame);
	void Disconnect(ID id);
	void DisconnectAll();
	virtual void Stop();
	static unsigned __stdcall AcceptThread(LPVOID arg);
	static unsigned __stdcall IOCPWorkerThread(LPVOID arg);
private:
	// Accept
	LONG lSessionNum_ = 0;
	LONG lMaxSession_;
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

	// Monitoring º¯¼ö
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

