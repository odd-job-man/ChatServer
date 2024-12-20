#include <WinSock2.h>
#include <stdio.h>
#include <Windows.h>
#include "NetSession.h"
#include "ChatServer.h"
#include "Packet.h"
#include "Player.h"
#include "Job.h"
#include "CMessageQ.h"
#include "ErrType.h"
#include "Logger.h"

#include "Parser.h"

#include "MemLog.h"
#include "Timer.h"
#include "ChattingUpdate.h"
#include "CMClient.h"
#include "CommonProtocol.h"

#pragma comment(lib,"pdh.lib")

template<typename T>
__forceinline T& IGNORE_CONST(const T& value)
{
	return const_cast<T&>(value);
}

extern CMessageQ g_MQ;

ChatServer g_ChatServer;

ChatServer::ChatServer()
	:NetServer{ L"ChatConfig.txt" }, SESSION_TIMEOUT_{ 0 }, PLAYER_TIMEOUT_{ 0 }, TICK_PER_FRAME_{ 0 }
{}

void ChatServer::Start()
{
	char* pStart;
	PARSER psr = CreateParser(L"ChatConfig.txt");

	GetValue(psr, L"USER_MAX", (PVOID*)&pStart, nullptr);
	Player::MAX_PLAYER_NUM = (short)_wtoi((LPCWSTR)pStart);

	GetValue(psr, L"TICK_PER_FRAME", (PVOID*)&pStart, nullptr);
	IGNORE_CONST(TICK_PER_FRAME_) = _wtoi((LPCWSTR)pStart);

	GetValue(psr, L"SESSION_TIMEOUT", (PVOID*)&pStart, nullptr);
	IGNORE_CONST(SESSION_TIMEOUT_) = (ULONGLONG)_wtoi64((LPCWSTR)pStart);
	GetValue(psr, L"PLAYER_TIMEOUT", (PVOID*)&pStart, nullptr);
	IGNORE_CONST(PLAYER_TIMEOUT_) = (ULONGLONG)_wtoi64((LPCWSTR)pStart);
	ReleaseParser(psr);

	Player::pPlayerArr = new Player[maxSession_];

	for (DWORD i = 0; i < IOCP_WORKER_THREAD_NUM_; ++i)
		ResumeThread(hIOCPWorkerThreadArr_[i]);

	ResumeThread(hAcceptThread_);

	pLanClient_ = new CMClient{ SERVERNUM::CHAT };
	pLanClient_->Start();

	ChattingUpdate* pChatUpdate = new ChattingUpdate{ (DWORD)TICK_PER_FRAME_,hcp_,5 };
	MonitoringUpdate* pMonitor = new MonitoringUpdate{ hcp_,1000,5 };
	Timer::Reigster_UPDATE(pChatUpdate);
	Timer::Reigster_UPDATE(pMonitor);
	pMonitor->RegisterMonitor(static_cast<const Monitorable*>(pChatUpdate));
	Timer::Start();
}

BOOL ChatServer::OnConnectionRequest()
{
    if (lSessionNum_ + 1 >= maxSession_)
        return FALSE;

    return TRUE;
}

void* ChatServer::OnAccept(ULONGLONG id)
{
	return nullptr;
}

void ChatServer::OnRelease(ULONGLONG id)
{
	Packet* pPacket = PACKET_ALLOC(Net);
	pPacket->sessionID_ = id;
	pPacket->recvType_ = JOB;
	*pPacket << (WORD)en_JOB_ON_RELEASE;
	MEMORY_LOG(ON_RELEASE, id);
	g_MQ.Enqueue(pPacket);
}

void ChatServer::OnRecv(ULONGLONG id, Packet* pPacket)
{
	pPacket->sessionID_= id;
	pPacket->recvType_ = RECVED_PACKET;
	g_MQ.Enqueue(pPacket);
}

void ChatServer::OnError(ULONGLONG id, int errorType, Packet* pRcvdPacket)
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

void ChatServer::OnPost(void* order)
{
	((Excutable*)order)->Excute();
}

void ChatServer::Monitoring()
{
	FILETIME ftCreationTime, ftExitTime, ftKernelTime, ftUsertTime;
	FILETIME ftCurTime;
	GetProcessTimes(GetCurrentProcess(), &ftCreationTime, &ftExitTime, &ftKernelTime, &ftUsertTime);
	GetSystemTimeAsFileTime(&ftCurTime);

	ULARGE_INTEGER start, now;
	start.LowPart = ftCreationTime.dwLowDateTime;
	start.HighPart = ftCreationTime.dwHighDateTime;
	now.LowPart = ftCurTime.dwLowDateTime;
	now.HighPart = ftCurTime.dwHighDateTime;


	ULONGLONG ullElapsedSecond = (now.QuadPart - start.QuadPart) / 10000 / 1000;

	ULONGLONG temp = ullElapsedSecond;

	ULONGLONG ullElapsedMin = ullElapsedSecond / 60;
	ullElapsedSecond %= 60;

	ULONGLONG ullElapsedHour = ullElapsedMin / 60;
	ullElapsedMin %= 60;

	ULONGLONG ullElapsedDay = ullElapsedHour / 24;
	ullElapsedHour %= 24;

	monitor.UpdateCpuTime(&PROCESS_CPU_TICK_ELAPSED, &PROCESS_CPU_TICK_TIME_DIFF);
	monitor.UpdateQueryData();

	ULONGLONG acceptTPS = InterlockedExchange(&acceptCounter_, 0);
	ULONGLONG disconnectTPS = InterlockedExchange(&disconnectTPS_, 0);
	ULONGLONG recvTPS = InterlockedExchange(&recvTPS_, 0);
	LONG sendTPS = InterlockedExchange(&sendTPS_, 0);
	LONG sessionNum = lSessionNum_;
	LONG playerNum = lPlayerNum;

	acceptTotal_ += acceptTPS;
	RECV_TOTAL += recvTPS;
	UPDATE_CNT_TOTAL += UPDATE_CNT_TPS;

	double processPrivateMByte = monitor.GetPPB() / (1024 * 1024);
	double nonPagedPoolMByte = monitor.GetNPB() / (1024 * 1024);
	double availableByte = monitor.GetAB();
	double networkSentBytes = monitor.GetNetWorkSendBytes();
	double networkRecvBytes = monitor.GetNetWorkRecvBytes();


	printf(
		"Elapsed Time : %02lluD-%02lluH-%02lluMin-%02lluSec\n"
		"MonitorServerConnected : %s\n"
		"update Count : %d\n"
		"Packet Pool Alloc Capacity : %d\n"
		"Packet Pool Alloc UseSize: %d\n"
		"MessageQ Capacity : %d\n"
		"MessageQ Queued By Worker : %d\n"
		"SendQ Pool Capacity : %d\n"
		"Accept TPS: %llu\n"
		"Accept Total : %llu\n"
		"Disconnect TPS: %llu\n"
		"Recv Msg TPS: %llu\n"
		"Send Msg TPS: %d\n"
		"Session Num : %d\n"
		"Player Num : %d\n"
		"----------------------\n"
		"Process Private MBytes : %.2lf\n"
		"Process NP Pool KBytes : %.2lf\n"
		"Memory Available MBytes : %.2lf\n"
		"Machine NP Pool MBytes : %.2lf\n"
		"Processor CPU Time : %.2f\n"
		"Process CPU Time : %.2f\n"
		"Process CPU Time AVR : %.2f\n"
		"TCP Retransmitted/sec : %.2f\n\n",
		ullElapsedDay, ullElapsedHour, ullElapsedMin, ullElapsedSecond,
		(pLanClient_->bLogin_ == TRUE) ? "True" : "False",
		UPDATE_CNT_TPS,
		Packet::packetPool_.capacity_ * Bucket<Packet, false>::size,
		Packet::packetPool_.AllocSize_,
		g_MQ.packetPool_.capacity_,
		g_MQ.packetPool_.AllocSize_,
		pSessionArr_[0].sendPacketQ_.nodePool_.capacity_,
		acceptTPS,
		acceptTotal_,
		disconnectTPS,
		recvTPS,
		sendTPS,
		sessionNum,
		lPlayerNum,
		processPrivateMByte,
		monitor.GetPNPB() / 1024,
		availableByte,
		nonPagedPoolMByte,
		monitor._fProcessorTotal,
		monitor._fProcessTotal,
		(float)(PROCESS_CPU_TICK_ELAPSED / (double)monitor._iNumberOfProcessors / (double)PROCESS_CPU_TICK_TIME_DIFF) * 100.0f,
		monitor.GetRetranse()
		);

	if (pLanClient_->bLogin_ == FALSE)
	{
		UPDATE_CNT_TPS = 0;
		return;
	}
	time_t curTime;
	time(&curTime);

#pragma warning(disable : 4244) // 프로토콜이 4바이트를 받고 상위4바이트는 버려서 별수없음
	pLanClient_->SendToMonitoringServer(CHAT, dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN, (int)1, curTime);
	pLanClient_->SendToMonitoringServer(CHAT, dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU, (int)monitor._fProcessTotal, curTime);
	pLanClient_->SendToMonitoringServer(CHAT, dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM, (int)processPrivateMByte, curTime);
	pLanClient_->SendToMonitoringServer(CHAT, dfMONITOR_DATA_TYPE_CHAT_SESSION, (int)sessionNum, curTime);
	pLanClient_->SendToMonitoringServer(CHAT, dfMONITOR_DATA_TYPE_CHAT_PLAYER, (int)playerNum, curTime);
	pLanClient_->SendToMonitoringServer(CHAT, dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS, (int)UPDATE_CNT_TPS, curTime);
	pLanClient_->SendToMonitoringServer(CHAT, dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL, (int)Packet::packetPool_.AllocSize_, curTime);
	pLanClient_->SendToMonitoringServer(CHAT, dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL, (int)g_MQ.packetPool_.size_, curTime);

	pLanClient_->SendToMonitoringServer(CHAT, dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL, (int)monitor._fProcessorTotal, curTime);
	pLanClient_->SendToMonitoringServer(CHAT, dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY, (int)nonPagedPoolMByte, curTime);
	pLanClient_->SendToMonitoringServer(CHAT, dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV, (int)(networkRecvBytes / (float)1024), curTime);
	pLanClient_->SendToMonitoringServer(CHAT, dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND, (int)(networkSentBytes / (float)1024), curTime);
	pLanClient_->SendToMonitoringServer(CHAT, dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY, (int)availableByte, curTime);
#pragma warning(default : 4244)

	UPDATE_CNT_TPS = 0;
}

void ChatServer::DisconnectAll()
{
	closesocket(hListenSock_);
	WaitForSingleObject(hAcceptThread_, INFINITE);
	CloseHandle(hAcceptThread_);

	LONG MaxSession = maxSession_;
	for (LONG i = 0; i < MaxSession; ++i)
	{
		// RELEASE 중이 아니라면 Disconnect
		pSessionArr_[i].bDisconnectCalled_ = true;
		CancelIoEx((HANDLE)pSessionArr_[i].sock_, nullptr);
	}

	while (lSessionNum_ != 0);
}
