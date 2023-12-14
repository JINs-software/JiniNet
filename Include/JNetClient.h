#pragma once
#include "JNetClientNetworkCore.h"
#include "JNetClientEventHandler.h"
#include "JNetProxy.h"
#include "JNetStub.h"
#include "JNetBatchProcess.h"

#define CLIENT_RECV_BUFF_SIZE 1'000
#define CLIENT_SEND_BUFF_SIZE 1'000

class JNetClient
{
private:
	JNetClientNetworkCore* networkCore;
	JNetBatchProcess* batchProcess;
	JBuffer* recvBuff;
	JBuffer* sendBuff;

public:
	JNetClient();
	inline void AttachEventHandler(JNetClientEventHandler* eventHandler);
	inline void AttachProxy(JNetProxy* proxy, BYTE unique);
	inline void AttachStub(JNetStub* stub, BYTE unique);
	inline void AttachBatchProcess(JNetBatchProcess* batch);
	inline void Connect();
	void FrameMove();
};

