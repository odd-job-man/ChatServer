#include <WinSock2.h>


#include "RingBuffer.h"

#include "Session.h"

BOOL Session::Init(SOCKET clientSock, ULONGLONG ullClientID, SHORT shIdx)
{
    sock_ = clientSock;
    bSendingInProgress_ = FALSE;
    id_ = ((ullClientID << 16) ^ shIdx);
    bDisconnectCalled_ = false;
    lSendBufNum_ = 0;
    recvRB_.ClearBuffer();
    return TRUE;
}
