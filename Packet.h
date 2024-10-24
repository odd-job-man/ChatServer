#pragma once
#include <memory.h>
#include "CLockFreeQueue.h"
#include "CLockFreeStack.h"
#include "RingBuffer.h"
#include "Session.h"
#include "IHandler.h"
//#include "LanServer.h"
#include "NetServer.h"

#define QUEUE
//#include "CTlsObjectPool.h"
#include "CLockFreeObjectPool.h"

using NET_HEADER = short;

enum RecvType 
{
	JOB,
	RECVED_PACKET
};

enum ServerType
{
	Net,
	Lan
};

class Packet
{
public:
#pragma pack(push,1)
	struct LanHeader
	{
		short payloadLen_;
	};

	struct NetHeader
	{
		unsigned char code_;
		unsigned short payloadLen_;
		unsigned char randomKey_;
		unsigned char checkSum_;
	};
#pragma pack(pop,1)

	static constexpr unsigned char FIXED_KEY = 0x32;
	static constexpr int RINGBUFFER_SIZE = 10000;
	static constexpr int BUFFER_SIZE = (RINGBUFFER_SIZE / 8) + sizeof(NetHeader);

	template<ServerType type>
	void Clear(void)
	{
		bEncoded_ = false;
		if constexpr (type == Net)
		{
			front_ = rear_ = sizeof(NetHeader);
		}
		else
		{
			front_ = rear_ = sizeof(LanHeader);
		}
	}

	int GetData(char* pDest, int sizeToGet)
	{
		if (rear_ - front_ < sizeToGet)
		{
			return 0;
		}
		else
		{
			memcpy_s(pDest, sizeToGet, pBuffer_ + front_, sizeToGet);
			front_ += sizeToGet;
			return sizeToGet;
		}
	}

	char* GetPointer(int sizeToGet)
	{
		if (rear_ - front_ < sizeToGet)
		{
			return 0;
		}
		else
		{
			char* pRet = pBuffer_ + front_;
			front_ += sizeToGet;
			return pRet;
		}
	}

	int PutData(char* pSrc, int sizeToPut)
	{
		memcpy_s(pBuffer_ + rear_, sizeToPut, pSrc, sizeToPut);
		rear_ += sizeToPut;
		return sizeToPut;
	}

	int GetUsedDataSize(void)
	{
		return rear_ - front_;
	}

	template<ServerType st>
	char* GetPayloadStartPos(void)
	{
		if constexpr (st == Lan)
		{
			return pBuffer_ + sizeof(LanHeader);
		}
		else
		{
			return pBuffer_ + sizeof(NetHeader);
		}
	}

	int MoveWritePos(int sizeToWrite)
	{
		rear_ += sizeToWrite;
		return sizeToWrite;
	}

	int MoveReadPos(int sizeToRead)
	{
		front_ += sizeToRead;
		return sizeToRead;
	}


	Packet& operator <<(const unsigned char value)
	{
		*(unsigned char*)(pBuffer_ + rear_) = value;
		rear_ += sizeof(value);
		return *this;
	}
	Packet& operator >>(unsigned char& value)
	{
		value = *(unsigned char*)(pBuffer_ + front_);
		front_ += sizeof(value);
		return *this;
	}

	Packet& operator <<(const char value)
	{
		*(char*)(pBuffer_ + rear_) = value;
		rear_ += sizeof(value);
		return *this;
	}
	Packet& operator >>(char& value)
	{
		value = *(char*)(pBuffer_ + front_);
		front_ += sizeof(value);
		return *this;
	}

	Packet& operator <<(const short value)
	{
		*(short*)(pBuffer_ + rear_) = value;
		rear_ += sizeof(value);
		return *this;
	}
	Packet& operator >>(short& value)
	{
		value = *(short*)(pBuffer_ + front_);
		front_ += sizeof(value);
		return *this;
	}

	Packet& operator <<(const unsigned short value)
	{
		*(unsigned short*)(pBuffer_ + rear_) = value;
		rear_ += sizeof(value);
		return *this;
	}
	Packet& operator >>(unsigned short& value)
	{
		value = *(unsigned short*)(pBuffer_ + front_);
		front_ += sizeof(value);
		return *this;
	}

	Packet& operator <<(const int value)
	{
		*(int*)(pBuffer_ + rear_) = value;
		rear_ += sizeof(value);
		return *this;
	}
	Packet& operator >>(int& value)
	{
		value = *(int*)(pBuffer_ + front_);
		front_ += sizeof(value);
		return *this;
	}

	Packet& operator <<(const unsigned int value)
	{
		*(unsigned int*)(pBuffer_ + rear_) = value;
		rear_ += sizeof(value);
		return *this;
	}
	Packet& operator >>(unsigned int& value)
	{
		value = *(unsigned int*)(pBuffer_ + front_);
		front_ += sizeof(value);
		return *this;
	}

	Packet& operator <<(const long value)
	{
		*(long*)(pBuffer_ + rear_) = value;
		rear_ += sizeof(value);
		return *this;
	}

	Packet& operator >>(long& value)
	{
		value = *(long*)(pBuffer_ + front_);
		front_ += sizeof(value);
		return *this;
	}

	Packet& operator <<(const unsigned long value)
	{
		*(unsigned long*)(pBuffer_ + rear_) = value;
		rear_ += sizeof(value);
		return *this;
	}
	Packet& operator >>(unsigned long& value)
	{
		value = *(unsigned long*)(pBuffer_ + front_);
		front_ += sizeof(value);
		return *this;
	}

	Packet& operator <<(const __int64 value)
	{
		*(__int64*)(pBuffer_ + rear_) = value;
		rear_ += sizeof(value);
		return *this;
	}

	Packet& operator >>(__int64& value)
	{
		value = *(__int64*)(pBuffer_ + front_);
		front_ += sizeof(value);
		return *this;
	}

	Packet& operator <<(const unsigned __int64 value)
	{
		*(unsigned __int64*)(pBuffer_ + rear_) = value;
		rear_ += sizeof(value);
		return *this;
	}
	Packet& operator >>(unsigned __int64& value)
	{
		value = *(unsigned __int64*)(pBuffer_ + front_);
		front_ += sizeof(value);
		return *this;
	}

	Packet& operator <<(const float value)
	{
		*(float*)(pBuffer_ + rear_) = value;
		rear_ += sizeof(value);
		return *this;
	}
	Packet& operator >>(float& value)
	{
		value = *(float*)(pBuffer_ + front_);
		front_ += sizeof(value);
		return *this;
	}

	Packet& operator <<(const double value)
	{
		*(double*)(pBuffer_ + rear_) = value;
		rear_ += sizeof(value);
		return *this;
	}

	Packet& operator >>(double& value)
	{
		value = *(double*)(pBuffer_ + front_);
		front_ += sizeof(value);
		return *this;
	}

	char* pBuffer_;
	int front_;
	int rear_;
	int refCnt_ = 0;
	bool bEncoded_;
public:
	unsigned short playerIdx_;
	RecvType recvType_;

#pragma warning(disable : 26495)
	Packet()
	{
		pBuffer_ = new char[BUFFER_SIZE];
	}
#pragma warning(default : 26495)

	~Packet()
	{
		delete[] pBuffer_;
	}

	__forceinline LONG IncreaseRefCnt()
	{
		return InterlockedIncrement((LONG*)&refCnt_);
	}
	__forceinline LONG DecrementRefCnt()
	{
		return InterlockedDecrement((LONG*)&refCnt_);
	}

	static unsigned char GetCheckSum(unsigned char* const pPayload, const int payloadLen)
	{
		unsigned char sum = 0;
		for (int i = 0; i < payloadLen; ++i)
		{
			sum += pPayload[i];
		}
		return sum % 256;
	}

	void Encode(NetHeader* pHeader)
	{
			char d = pHeader->checkSum_;
			char p = d ^ (pHeader->randomKey_ + 1);
			char e = p ^ (FIXED_KEY + 1);
			pHeader->checkSum_ = e;

			char* payload = pBuffer_ + sizeof(NetHeader);
			for (int i = 0; i < pHeader->payloadLen_; i++)
			{
				d = payload[i];
				p = d ^ (p + pHeader->randomKey_+ 2 + i);
				e = p ^ (e + FIXED_KEY + 2 + i);
				payload[i] = e;
			}
	}

	void Decode(NetHeader* pHeader)
	{
		int len = pHeader->payloadLen_ + sizeof(NetHeader::checkSum_);
		unsigned char* const pDecodeTarget = (unsigned char*)pHeader + offsetof(NetHeader, checkSum_);

		for (int i = len - 1; i >= 1; --i)
		{
			pDecodeTarget[i] = (pDecodeTarget[i - 1] + FIXED_KEY + i + 1) ^ pDecodeTarget[i];
		}

		pDecodeTarget[0] ^= (FIXED_KEY + 1);

		for (int i = len - 1; i >= 1; --i)
		{
			pDecodeTarget[i] = (pDecodeTarget[i - 1] + pHeader->randomKey_ + i + 1) ^ pDecodeTarget[i];
		}

		pDecodeTarget[0] ^= (pHeader->randomKey_ + 1);
	}

	template<ServerType st>
	void SetHeader()
	{
		if constexpr (st == Net)
		{
			if (bEncoded_ == true)
				return;

			// 헤더 설정
			NetHeader* pHeader = (NetHeader*)pBuffer_;
			pHeader->code_ = 0x77;
			pHeader->payloadLen_ = rear_ - front_;
			pHeader->randomKey_ = rand();
			pHeader->checkSum_ = GetCheckSum((unsigned char*)pHeader + sizeof(NetHeader), pHeader->payloadLen_);

			Encode(pHeader);
			bEncoded_ = true;
		}
		else
		{
			((LanHeader*)pBuffer_)->payloadLen_ = rear_ - front_;
		}
	}

	// 넷서버에서만 호출
	bool ValidateReceived()
	{
		// 아예 얼토당토않은 패킷이 왓을때를 위한 검증
		NetHeader* pHeader = (NetHeader*)pBuffer_;
		if (pHeader->code_ != 0x77)
			return false;

		Decode(pHeader);

		if (pHeader->checkSum_ != GetCheckSum((unsigned char*)pHeader + sizeof(NetHeader), pHeader->payloadLen_))
			return false;

		return true;
	}

	template<ServerType type>
	static Packet* Alloc()
	{
		Packet* pPacket = packetPool.Alloc();
		pPacket->Clear<type>();
		return pPacket;
	}


	static void Free(Packet* pPacket)
	{
		packetPool.Free(pPacket);
	}

	//static inline CTlsObjectPool<Packet, false> packetPool;
	static inline CLockFreeObjectPool<Packet, false> packetPool;

	friend class SmartPacket;
	//friend void LanServer::RecvProc(Session* pSession, int numberOfBytesTransferred);
	friend void NetServer::RecvProc(Session* pSession, int numberOfBytesTransferred);
	friend BOOL NetServer::SendPost(Session* pSession);
	friend void NetServer::SendProc(Session* pSession, DWORD dwNumberOfBytesTransferred);
	friend void NetServer::ReleaseSession(Session* pSession);
};

class SmartPacket
{
public:
	Packet* pPacket_;
	SmartPacket(Packet*&& pPacket)
		:pPacket_{pPacket}
	{
		if (pPacket == nullptr)
			return;

		pPacket_->IncreaseRefCnt();
	}

	~SmartPacket()
	{
		if (pPacket_ == nullptr)
			return;

		if(pPacket_->DecrementRefCnt() == 0)
		{
			Packet::Free(pPacket_);
		}
	}

	__forceinline Packet& operator*()
	{
		return *pPacket_;
	}

	__forceinline Packet* operator->()
	{
		return pPacket_;
	}

	__forceinline Packet* GetPacket() 
	{
		return pPacket_;
	}
};

#undef QUEUE
