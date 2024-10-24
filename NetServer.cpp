#include <Winsock2.h>
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include "CLockFreeQueue.h"
#include "RingBuffer.h"
#include "Session.h"
#include "IHandler.h"
#include "NetServer.h"
#include "Logger.h"
#include "Packet.h"
#include "CMessageQ.h"
#include "Job.h"
#include "Player.h"
#include "ErrType.h"
#pragma comment(lib,"LoggerMt.lib")
#pragma comment(lib, "ws2_32.lib")
#include "Logger.h"
#include "MemLog.h"


CMessageQ g_MQ;
#define SERVERPORT 11402
#define LINGER
#define ZERO_BYTE_SEND


BOOL NetServer::Start(DWORD dwMaxSession)
{
	int retval;
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		LOG(L"ONOFF", SYSTEM, TEXTFILE, L"WSAStartUp Fail ErrCode : %u", WSAGetLastError());
		__debugbreak();
	}
	LOG(L"ONOFF", SYSTEM, TEXTFILE, L"WSAStartUp OK!");
	// NOCT�� 0���� �����μ��� ����ŭ�� ������
	hcp_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	if (!hcp_)
	{
		LOG(L"ONOFF", SYSTEM, TEXTFILE, L"CreateIoCompletionPort Fail ErrCode : %u", WSAGetLastError());
		__debugbreak();
	}
	LOG(L"ONOFF", SYSTEM, TEXTFILE, L"Create IOCP OK!");

	SYSTEM_INFO si;
	GetSystemInfo(&si);

	hListenSock_ = socket(AF_INET, SOCK_STREAM, 0);
	if (hListenSock_ == INVALID_SOCKET)
	{
		LOG(L"ONOFF", SYSTEM, TEXTFILE, L"MAKE Listen SOCKET Fail ErrCode : %u", WSAGetLastError());
		__debugbreak();
	}
	LOG(L"ONOFF", SYSTEM, TEXTFILE, L"MAKE Listen SOCKET OK");

	// bind
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(hListenSock_, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR)
	{
		LOG(L"ONOFF", SYSTEM, TEXTFILE, L"bind Fail ErrCode : %u", WSAGetLastError());
		__debugbreak();
	}
	LOG(L"ONOFF", SYSTEM, TEXTFILE, L"bind OK");

	// listen
	retval = listen(hListenSock_, SOMAXCONN);
	if (retval == SOCKET_ERROR)
	{
		LOG(L"ONOFF", SYSTEM, TEXTFILE, L"listen Fail ErrCode : %u", WSAGetLastError());
		__debugbreak();
	}
	LOG(L"ONOFF", SYSTEM, TEXTFILE, L"listen() OK");

#ifdef LINGER
	linger linger;
	linger.l_linger = 0;
	linger.l_onoff = 1;
	setsockopt(hListenSock_, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));
	LOG(L"ONOFF", SYSTEM, TEXTFILE, L"linger() OK");
#endif

#ifdef ZERO_BYTE_SEND
	DWORD dwSendBufSize = 0;
	setsockopt(hListenSock_, SOL_SOCKET, SO_SNDBUF, (char*)&dwSendBufSize, sizeof(dwSendBufSize));
	LOG(L"ONOFF", SYSTEM, TEXTFILE, L"Zerobyte Send OK");
#endif

	hIOCPWorkerThreadArr_ = new HANDLE[si.dwNumberOfProcessors * 2];
	for (DWORD i = 0; i < si.dwNumberOfProcessors * 2; ++i)
	{
		hIOCPWorkerThreadArr_[i] = (HANDLE)_beginthreadex(NULL, 0, IOCPWorkerThread, this, 0, nullptr);
		if (!hIOCPWorkerThreadArr_[i])
		{
			LOG(L"ONOFF", SYSTEM, TEXTFILE, L"MAKE WorkerThread Fail ErrCode : %u", WSAGetLastError());
			__debugbreak();
		}
	}
	LOG(L"ONOFF", SYSTEM, TEXTFILE, L"MAKE IOCP WorkerThread OK Num : %u!", si.dwNumberOfProcessors);

	// ���� 17��Ʈ�� ������ ������Ʈ�� 16�� ���ϰ� �Ǵ³����� ������� ū�׸��̴�.
	if (!CAddressTranslator::CheckMetaCntBits())
		__debugbreak();

	pSessionArr_ = new Session[dwMaxSession];
	lMaxSession_ = dwMaxSession;
	for (int i = dwMaxSession - 1; i >= 0; --i)
	{
		DisconnectStack_.Push(i);
	}

	hAcceptThread_ = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, this, 0, nullptr);
	if (!hAcceptThread_)
	{
		LOG(L"ONOFF", SYSTEM, TEXTFILE, L"MAKE AccpetThread Fail ErrCode : %u", WSAGetLastError());
		__debugbreak();
	}
	LOG(L"ONOFF", SYSTEM, TEXTFILE, L"MAKE AccpetThread OK!");
	return 0;
}

void NetServer::SendPacket(ID id, SmartPacket& sendPacket)
{
	Session* pSession = pSessionArr_ + Session::GET_SESSION_INDEX(id);
	long IoCnt = InterlockedIncrement(&pSession->IoCnt_);

	// �̹� RELEASE �������̰ų� RELEASE�� ���
	if ((IoCnt & Session::RELEASE_FLAG) == Session::RELEASE_FLAG)
	{
		if (InterlockedDecrement(&pSession->IoCnt_) == 0)
		{
#ifdef MEMLOG
			MemoryLog(SENDPACKET_RELEASE_TRY_1, id);
#endif
			ReleaseSession(pSession);
		}
		return;
	}

	// RELEASE �Ϸ��� �ٽ� ���ǿ� ���� �ʱ�ȭ�� �Ϸ�Ȱ�� �� ��Ȱ��
	if (id.ullId != pSession->id_.ullId)
	{
		if (InterlockedDecrement(&pSession->IoCnt_) == 0)
		{
#ifdef MEMLOG
			MemoryLog(SENDPACKET_RELEASE_TRY_2, pSession->id_);
#endif
			ReleaseSession(pSession);
		}
		return;
	}

	// ���ڵ�
	sendPacket->SetHeader<Net>();
	sendPacket->IncreaseRefCnt();
	pSession->sendPacketQ_.Enqueue(sendPacket.GetPacket());
	SendPost(pSession);
	if (InterlockedDecrement(&pSession->IoCnt_) == 0)
	{
#ifdef MEMLOG
		MemoryLog(SENDPACKET_RELEASE_TRY_3, pSession->id_);
#endif
		ReleaseSession(pSession);
	}
}

BOOL NetServer::OnConnectionRequest()
{
	if (lSessionNum_ + 1 >= lMaxSession_)
		return FALSE;

	return TRUE;
}

void* NetServer::OnAccept(ID id)
{
	Packet* pPacket = Packet::Alloc<Net>();
	pPacket->playerIdx_ = Player::MAKE_PLAYER_INDEX(id);
	pPacket->recvType_ = JOB;
	*pPacket << (WORD)en_JOB_ON_ACCEPT << id.ullId;
#ifdef MEMLOG
	MemoryLog(ON_ACCEPT, id);
#endif
	g_MQ.Enqueue(pPacket);
	return nullptr;
}

void NetServer::OnRelease(ID id)
{
	Packet* pPacket = Packet::Alloc<Net>();
	pPacket->playerIdx_ = Session::GET_SESSION_INDEX(id);
	pPacket->recvType_ = JOB;
	*pPacket << (WORD)en_JOB_ON_RELEASE;
#ifdef MEMLOG
	MemoryLog(ON_RELEASE, id);
#endif
	g_MQ.Enqueue(pPacket);
}

void NetServer::OnRecv(ID id, Packet* pPacket)
{
	pPacket->playerIdx_ = Player::MAKE_PLAYER_INDEX(id);
	pPacket->recvType_ = RECVED_PACKET;
	g_MQ.Enqueue(pPacket);
}

void NetServer::OnError(ID id, int errorType, Packet* pRcvdPacket)
{
	switch ((ErrType)errorType)
	{
	case PACKET_PROC_RECVED_PACKET_INVALID_TYPE:
	{
		pRcvdPacket->MoveReadPos(-(int)sizeof(WORD));
		WORD type;
		*pRcvdPacket >> type;
		LOG(L"PACKET_PROC_RECVED_PACKET_INVALID_TYPE", SYSTEM, TEXTFILE, L"Session ID : %d Recved Type : %x", id, type);
		break;
	}
	default:
		__debugbreak();
		break;
	}
}

void NetServer::Monitoring(int updateCnt)
{
	printf(
		"update Count : %d\n"
		"Packet Pool Alloc Cnt : %d\n"
		"Accept TPS: %d\n"
		"Accept Total : %d\n"
		"Disconnect TPS: %d\n"
		"Recv Msg TPS: %d\n"
		"Send Msg TPS: %d\n"
		"Session Num : %d\n"
		"Player Num : %d\n\n",
		updateCnt, Packet::packetPool.capacity_ - Packet::packetPool.size_, lAcceptTotal_ - lAcceptTotal_PREV, lAcceptTotal_, lDisconnectTPS_, lRecvTPS_, lSendTPS_, lSessionNum_, lPlayerNum);

	lAcceptTotal_PREV = lAcceptTotal_;
	InterlockedExchange(&lDisconnectTPS_, 0);
	InterlockedExchange(&lRecvTPS_, 0);
	InterlockedExchange(&lSendTPS_, 0);
}

void NetServer::Disconnect(ID id)
{
	__debugbreak();
	Session* pSession = pSessionArr_ + Session::GET_SESSION_INDEX(id);

	long IoCnt = InterlockedIncrement(&pSession->IoCnt_);

	// RELEASE������ Ȥ�� ����Ϸ�
	if ((IoCnt & Session::RELEASE_FLAG) == Session::RELEASE_FLAG)
	{
		if (InterlockedDecrement(&pSession->IoCnt_) == 0)
		{
			ReleaseSession(pSession);
			return;
		}
	}

	// RELEASE�� ��Ȱ����� �Ǿ�����
	if (id.ullId != pSession->id_.ullId)
	{
		if (InterlockedDecrement(&pSession->IoCnt_) == 0)
		{
			ReleaseSession(pSession);
			return;
		}
	}

	// Disconnect 1ȸ ����
	if (pSession->bDisconnectCalled_ == true)
	{
		if (InterlockedDecrement(&pSession->IoCnt_) == 0)
		{
			ReleaseSession(pSession);
			return;
		}
	}

	// ���� �����޴ٸ� ���� ���ǿ� ���ؼ� RELEASE ���� ȣ����� ������������ ����ȴ�
	pSession->bDisconnectCalled_ = true;
	CancelIoEx((HANDLE)pSession->sock_, nullptr);

	// CancelIoExȣ��� ���ؼ� RELEASE�� ȣ��Ǿ���� ������ �������� InterlockedIncrement ������ ȣ���� �ȵ� ��� ����û��
	if (InterlockedDecrement(&pSession->IoCnt_) == 0)
		ReleaseSession(pSession);
}

void NetServer::DisconnectAll()
{
	closesocket(hListenSock_);
	WaitForSingleObject(hAcceptThread_, INFINITE);
	CloseHandle(hAcceptThread_);

	LONG MaxSession = lMaxSession_;
	for (LONG i = 0; i < MaxSession; ++i)
	{
		// RELEASE ���� �ƴ϶�� Disconnect
		pSessionArr_[i].bDisconnectCalled_ = true;
		CancelIoEx((HANDLE)pSessionArr_[i].sock_, nullptr);
	}
	while (lSessionNum_!= 0);
}

void NetServer::Stop()
{
	for (LONG i = 0; i < lMaxSession_; ++i)
	{
		//pSessionArr_[i].sendPacketQ_.nodePool_.ClearAll();
	}

	SYSTEM_INFO si;
	GetSystemInfo(&si);
	for (DWORD i = 0; i < si.dwNumberOfProcessors * 2; ++i)
	{
		PostQueuedCompletionStatus(hcp_, 0, 0, 0);
	}
	WaitForMultipleObjects(si.dwNumberOfProcessors * 2, hIOCPWorkerThreadArr_, TRUE, INFINITE);
	for (DWORD i = 0; i < si.dwNumberOfProcessors * 2; ++i)
	{
		CloseHandle(hIOCPWorkerThreadArr_[i]);
	}

	delete[] pSessionArr_;

	// ����ȭ ���� Ǯ ����
	//Packet::packetPool.ClearAll();

	// DisconnectStack ����
	while (DisconnectStack_.Pop().has_value());
	// DisconnectStack Ǯ ����
	//DisconnectStack_.pool_.ClearAll();

	//while (g_MQ.Dequeue().has_value());
	// �޽��� ť�� ��� Ǯ ����
	//g_MQ.nodePool_.ClearAll();

}

BOOL NetServer::RecvPost(Session* pSession)
{
	if (pSession->bDisconnectCalled_ == true)
		return FALSE;

	WSABUF wsa[2];
	wsa[0].buf = pSession->recvRB_.GetWriteStartPtr();
	wsa[0].len = pSession->recvRB_.DirectEnqueueSize();
	wsa[1].buf = pSession->recvRB_.Buffer_;
	wsa[1].len = pSession->recvRB_.GetFreeSize() - wsa[0].len;

	ZeroMemory(&pSession->recvOverlapped, sizeof(WSAOVERLAPPED));
	DWORD flags = 0;
	InterlockedIncrement(&pSession->IoCnt_);
	int iRecvRet = WSARecv(pSession->sock_, wsa, 2, nullptr, &flags, &(pSession->recvOverlapped), nullptr);
	if (iRecvRet == SOCKET_ERROR)
	{
		DWORD dwErrCode = WSAGetLastError();
		if (dwErrCode == WSA_IO_PENDING)
		{
			if (pSession->bDisconnectCalled_ == true)
			{
				InterlockedDecrement(&(pSession->IoCnt_));
				CancelIoEx((HANDLE)pSession->sock_, nullptr);
				return FALSE;
			}
			return TRUE;
		}

		InterlockedDecrement(&(pSession->IoCnt_));
		if (dwErrCode == WSAECONNRESET)
			return FALSE;

		LOG(L"Disconnect", ERR, TEXTFILE, L"Client Disconnect By ErrCode : %u", dwErrCode);
		return FALSE;
	}
	return TRUE;
}

BOOL NetServer::SendPost(Session* pSession)
{
	if (pSession->bDisconnectCalled_ == true)
		return TRUE;

	// ���� ���� TRUE�� �ٲ۴�. ���� TRUE���ٸ� ��ȯ���� TRUE�ϰ��̸� �׷��ٸ� ���� SEND �������̱� ������ �׳� ����������
	// �� ���ǹ��� ��ġ�� ���Ͽ� Out�� �ٲ��� ���������� ����ȴ�.
	// ������ SendPost ������ü�� Send�Ϸ����� �������� ��쿡�� in�� ��ġ�� SendPacket���� ���ؼ� �ٲ���� �ִ�.
	// iUseSize�� ���ϴ� ���������� DirectDequeueSize�� ���� �޶������ִ�.
	if (InterlockedExchange((LONG*)&pSession->bSendingInProgress_, TRUE) == TRUE)
		return TRUE;

	// SendPacket���� in�� �Űܼ� UseSize�� 0���� Ŀ���������� Send�Ϸ������� �����ؼ� Out�� �ű�� �÷��� ���� Recv�Ϸ����� �����尡 ���� SendPost�� ������ �÷��׸� �����Ѱ�� UseSize�� 0�̳��´�.
	// ���⼭ flag�� �ٽ� FALSE�� �ٲپ����� �ʾƼ� ����߻�
	DWORD dwBufferNum = pSession->sendPacketQ_.GetSize();
	if (dwBufferNum == 0)
	{
		InterlockedExchange((LONG*)&pSession->bSendingInProgress_, FALSE);
		return TRUE;
	}

	WSABUF wsa[50];
	DWORD i;
	for (i = 0; i < 50 && i < dwBufferNum; ++i)
	{
		Packet* pPacket = pSession->sendPacketQ_.Dequeue().value();
		wsa[i].buf = (char*)pPacket->pBuffer_;
		wsa[i].len = pPacket->GetUsedDataSize() + sizeof(Packet::NetHeader);
		pSession->pSendPacketArr_[i] = pPacket;
	}

	InterlockedExchange(&pSession->lSendBufNum_, i);
	InterlockedAdd(&lSendTPS_, i);
	InterlockedIncrement(&pSession->IoCnt_);
	ZeroMemory(&(pSession->sendOverlapped), sizeof(WSAOVERLAPPED));
	int iSendRet = WSASend(pSession->sock_, wsa, i, nullptr, 0, &(pSession->sendOverlapped), nullptr);
	if (iSendRet == SOCKET_ERROR)
	{
		DWORD dwErrCode = WSAGetLastError();
		if (dwErrCode == WSA_IO_PENDING)
		{
			if (pSession->bDisconnectCalled_ == true)
			{
				CancelIoEx((HANDLE)pSession->sock_, nullptr);
				//InterlockedExchange((LONG*)&pSession->bSendingInProgress_, FALSE);
				return FALSE;
			}
			return TRUE;
		}

		InterlockedExchange((LONG*)&pSession->bSendingInProgress_, FALSE);
		InterlockedDecrement(&(pSession->IoCnt_));
		if (dwErrCode == WSAECONNRESET)
			return FALSE;

		LOG(L"Disconnect", ERR, TEXTFILE, L"Client Disconnect By ErrCode : %u", dwErrCode);
		return FALSE;
	}
	return TRUE;
}

void NetServer::ReleaseSession(Session* pSession)
{
	if (InterlockedCompareExchange(&pSession->IoCnt_, Session::RELEASE_FLAG | 0, 0) != 0)
		return;

#ifdef MEMLOG
	MemoryLog(RELEASE_SESSION, pSession->id_);
#endif

	// Release �� Session�� ����ȭ ���� ����
	for (LONG i = 0; i < pSession->lSendBufNum_; ++i)
	{
		Packet* pPacket = pSession->pSendPacketArr_[i];
		if (pPacket->DecrementRefCnt() == 0)
		{
			Packet::Free(pPacket);
		}
	}

	for (LONG i = 0; pSession->sendPacketQ_.GetSize(); ++i)
	{
		Packet* pPacket = pSession->sendPacketQ_.Dequeue().value();
		if (pPacket->DecrementRefCnt() == 0)
		{
			Packet::Free(pPacket);
		}
	}

	closesocket(pSession->sock_);
	if (pSession->sendPacketQ_.GetSize() > 0)
		__debugbreak();

	// OnRelease�� idx Ǫ�� ������ �ٲ� JOB_OnRelease ���� ������ ���ο� �÷��̾ ���� JOB_On_ACCEPT�� �ߺ����� ��������
	OnRelease(pSession->id_);
	DisconnectStack_.Push((short)(pSession - pSessionArr_));
	InterlockedIncrement(&lDisconnectTPS_);
	InterlockedDecrement(&lSessionNum_);
}

void NetServer::RecvProc(Session* pSession, int numberOfBytesTransferred)
{
	using NetHeader = Packet::NetHeader;
	pSession->recvRB_.MoveInPos(numberOfBytesTransferred);
	while (1)
	{
		if (pSession->bDisconnectCalled_ == true)
			return;

		Packet* pPacket = Packet::Alloc<Net>();
		if (pSession->recvRB_.Peek(pPacket->pBuffer_, sizeof(NetHeader)) == 0)
		{
			Packet::Free(pPacket);
			break;
		}

		int payloadLen = ((NetHeader*)pPacket->pBuffer_)->payloadLen_;

		if (pSession->recvRB_.GetUseSize() < sizeof(NetHeader) + payloadLen)
		{
			Packet::Free(pPacket);
			break;
		}

		pSession->recvRB_.MoveOutPos(sizeof(NetHeader));
		pSession->recvRB_.Dequeue(pPacket->GetPayloadStartPos<Net>(), payloadLen);
		pPacket->MoveWritePos(payloadLen);

		// �ݼ��������� ȣ��Ǵ� �Լ��� ���� �� ���ڵ��� üũ�� Ȯ��
		if (pPacket->ValidateReceived() == false)
		{
			Packet::Free(pPacket);
			Disconnect(pSession->id_);
			return;
		}
		OnRecv(pSession->id_, pPacket);
		InterlockedIncrement(&lRecvTPS_);
	}
	RecvPost(pSession);
}

void NetServer::SendProc(Session* pSession, DWORD dwNumberOfBytesTransferred)
{
	LONG sendBufNum = pSession->lSendBufNum_;
	pSession->lSendBufNum_ = 0;
	for (LONG i = 0; i < sendBufNum; ++i)
	{
		Packet* pPacket = pSession->pSendPacketArr_[i];
		if (pPacket->DecrementRefCnt() == 0)
		{
			Packet::Free(pPacket);
		}
	}

	InterlockedExchange((LONG*)&pSession->bSendingInProgress_, FALSE);
	if (pSession->sendPacketQ_.GetSize() > 0)
		SendPost(pSession);
}

unsigned __stdcall NetServer::AcceptThread(LPVOID arg)
{
	SOCKET clientSock;
	SOCKADDR_IN clientAddr;
	int addrlen;
	NetServer* pNetServer = (NetServer*)arg;
	addrlen = sizeof(clientAddr);

	while (1)
	{
		clientSock = accept(pNetServer->hListenSock_, (SOCKADDR*)&clientAddr, &addrlen);
		if (clientSock == INVALID_SOCKET)
		{
			DWORD dwErrCode = WSAGetLastError();
			if (dwErrCode != WSAEINTR && dwErrCode != WSAENOTSOCK)
			{
				__debugbreak();
			}
			return 0;
		}

		if (!pNetServer->OnConnectionRequest())
		{
			closesocket(clientSock);
			continue;
		}

		InterlockedIncrement((LONG*)&pNetServer->lAcceptTotal_);
		InterlockedIncrement((LONG*)&pNetServer->lSessionNum_);

		short idx = pNetServer->DisconnectStack_.Pop().value();
		Session* pSession = pNetServer->pSessionArr_ + idx;
		pSession->Init(clientSock, pNetServer->ullIdCounter, idx);

		CreateIoCompletionPort((HANDLE)pSession->sock_, pNetServer->hcp_, (ULONG_PTR)pSession, 0);
		++pNetServer->ullIdCounter;

		InterlockedIncrement(&pSession->IoCnt_);
		InterlockedAnd(&pSession->IoCnt_, ~Session::RELEASE_FLAG);

		pNetServer->OnAccept(pSession->id_);
		pNetServer->RecvPost(pSession);

		if (InterlockedDecrement(&pSession->IoCnt_) == 0)
			pNetServer->ReleaseSession(pSession);
	}
	return 0;
}

unsigned __stdcall NetServer::IOCPWorkerThread(LPVOID arg)
{
	NetServer* pNetServer = (NetServer*)arg;
	while (1)
	{
		WSAOVERLAPPED* pOverlapped = nullptr;
		DWORD dwNOBT = 0;
		Session* pSession = nullptr;
		BOOL bGQCSRet = GetQueuedCompletionStatus(pNetServer->hcp_, &dwNOBT, (PULONG_PTR)&pSession, (LPOVERLAPPED*)&pOverlapped, INFINITE);
		do
		{
			if (!pOverlapped && !dwNOBT && !pSession)
				return 0;

			//��������
			if (bGQCSRet && dwNOBT == 0)
				break;

			//������ ����
			//�α��� �Ϸ������� GQCS ���� WSAGetLastError ���� 64�� ���� �������.
			//���� WSASend�� WSARecv, �� �� �ϳ��� �ٷ� �����ϴ� ��쿡�� �α��ϴ°�����...
			if (!bGQCSRet && pOverlapped)
				break;

			if (&pSession->recvOverlapped == pOverlapped)
				pNetServer->RecvProc(pSession, dwNOBT);
			else
				pNetServer->SendProc(pSession, dwNOBT);

		} while (0);

		if (InterlockedDecrement(&pSession->IoCnt_) == 0)
			pNetServer->ReleaseSession(pSession);
	}
	return 0;
}
