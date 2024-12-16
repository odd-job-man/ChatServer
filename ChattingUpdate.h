#pragma once
#include "MyJob.h"
#include "UpdateBase.h"

class ChattingUpdate : public UpdateBase
{
public:
	ChattingUpdate(DWORD tickPerFrame, HANDLE hCompletionPort, LONG pqcsLimit);
	virtual void Update_IMPL() override;
	virtual void OnMonitor() override;
};
