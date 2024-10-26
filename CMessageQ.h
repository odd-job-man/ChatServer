#pragma once
#include <cstdint>
#include "CAddressTranslator.h"
#include "Packet.h"
class CMessageQ
{
public:
	struct Node
	{
		Packet* data_;
		Node* pNext_;

#pragma warning(disable : 26495)
		Node()
			:pNext_{ nullptr }
		{}
#pragma warning(default : 26495)

		Node(Packet* data)
			:data_{ data },
			pNext_{ nullptr }
		{}
	};

	CTlsObjectPool<Node, true> packetPool_;
	Node* pTailForWorker_;
	Node* pHeadForWorker_;

	Node* pTailForSingle_;
	Node* pHeadForSingle_;
public:
	unsigned long long BuffersToProcessThisFrame_ = 0;
	alignas(64) unsigned long long workerEnqueuedBufferCnt_ = 0;

#ifdef NO_LOCK
	static constexpr uint64_t SWAP_FLAG = 0x8000000000000000;
	uint64_t SWAP_GUARD;
#else
	SRWLOCK SWAP_GUARD;
#endif
public:
	CMessageQ();
	// meta가 붙으면 상위 17비트가 조작된 값
	// real이 붙으면 상위 17비트를 제거한 값
	void Enqueue(Packet* data);
	Packet* Dequeue();
	void Swap();
	void ClearAll();
};
