#pragma once
#include "MyJob.h"
#include "UpdateBase.h"
#include "Monitorable.h"

class ChattingUpdate : public UpdateBase, public Monitorable
{
public:
	ChattingUpdate(DWORD tickPerFrame, HANDLE hCompletionPort, LONG pqcsLimit);
	virtual void Update_IMPL() override;
	virtual void OnMonitor() override;
};
