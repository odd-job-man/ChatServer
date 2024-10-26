#include <WinSock2.h>
#include "CMessageQ.h"

CMessageQ::CMessageQ()
{
		pTailForWorker_ = pHeadForWorker_ = packetPool_.Alloc();
		pTailForSingle_ = pHeadForSingle_ = packetPool_.Alloc();
#ifdef NO_LOCK
		SWAP_GUARD = 0;
#else
		InitializeSRWLock(&SWAP_GUARD);
#endif
}

void CMessageQ::Enqueue(Packet* data)
{
#ifdef NO_LOCK
	while ((InterlockedIncrement(&SWAP_GUARD) & SWAP_FLAG) == SWAP_FLAG)
	{
		YieldProcessor();
		InterlockedDecrement(&SWAP_GUARD);
	}
#else
	AcquireSRWLockShared(&SWAP_GUARD);
#endif

	Node* pNewNode = packetPool_.Alloc(data);
	while (true)
	{
		Node* pTail = pTailForWorker_;
		Node* pNextOfTail = pTail->pNext_;
		if (pNextOfTail != nullptr)
		{
			InterlockedCompareExchangePointer((PVOID*)&pTailForWorker_, (PVOID)pNextOfTail, (PVOID)pTail);
			continue;
		}

		if (InterlockedCompareExchangePointer((PVOID*)&pTail->pNext_, pNewNode, nullptr) != nullptr)
			continue;

		InterlockedCompareExchangePointer((PVOID*)&pTailForWorker_, (PVOID)pNewNode, (PVOID)pTail);
		InterlockedIncrement(&workerEnqueuedBufferCnt_);
		break;
	}

#ifdef NO_LOCK
	InterlockedDecrement(&SWAP_GUARD);
#else
	ReleaseSRWLockShared(&SWAP_GUARD);
#endif
}

// 싱글스레드, 업데이트 스레드가 호출해야한다
Packet* CMessageQ::Dequeue()
{
	if (pHeadForSingle_ == pTailForSingle_)
		return nullptr;

	Node* pFree = pHeadForSingle_;
	pHeadForSingle_ = pFree->pNext_;
	packetPool_.Free(pFree);
	--BuffersToProcessThisFrame_;
	return pHeadForSingle_->data_;
}

void CMessageQ::Swap()
{
#ifdef NO_LOCK
	while (InterlockedCompareExchange(&SWAP_GUARD, SWAP_FLAG | 0, 0) != 0)
		YieldProcessor();
#else
	AcquireSRWLockExclusive(&SWAP_GUARD);
#endif

	if (pHeadForSingle_ != pTailForSingle_)
		__debugbreak();

	// 더미노드
	Node* pTemp = pTailForSingle_;
	pTailForSingle_ = pTailForWorker_;
	pHeadForSingle_ = pHeadForWorker_;
	pTailForWorker_ = pHeadForWorker_ = pTemp;
	if (pTailForWorker_ == nullptr)
		__debugbreak();

	unsigned long long BufferCntTemp;
	BufferCntTemp = workerEnqueuedBufferCnt_;
	workerEnqueuedBufferCnt_ = BuffersToProcessThisFrame_;
	BuffersToProcessThisFrame_ = BufferCntTemp;
#ifdef NO_LOCK
	InterlockedAnd(&SWAP_GUARD, ~SWAP_FLAG);
#else
	ReleaseSRWLockExclusive(&SWAP_GUARD);
#endif
}

void CMessageQ::ClearAll()
{
	if (pTailForWorker_ != pHeadForWorker_)
		__debugbreak();
	if (pTailForSingle_ != pHeadForSingle_)
		__debugbreak();

	packetPool_.Free(pHeadForSingle_);
	packetPool_.Free(pHeadForWorker_);
}
