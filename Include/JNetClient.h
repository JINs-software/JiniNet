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
	JNetClient() {
		networkCore = new JNetClientNetworkCore();
		batchProcess = new JNetBatchProcess();
		
		recvBuff = new JBuffer(CLIENT_RECV_BUFF_SIZE);
		sendBuff = new JBuffer(CLIENT_SEND_BUFF_SIZE);
	}
	inline void AttachEventHandler(JNetClientEventHandler* eventHandler) {
		networkCore->AttachEventHandler(eventHandler);
	}
	inline void AttachProxy(JNetProxy* proxy, BYTE unique) {
		proxy->netcore = networkCore;
		proxy->uniqueNum = unique;
	}
	inline void AttachStub(JNetStub* stub, BYTE unique) {
		networkCore->AttachStub(stub);
		stub->uniqueNum = unique;
	}
	inline void AttachBatchProcess(JNetBatchProcess* batch) {
		if (!batchProcess) {
			delete batchProcess;
		}
		batchProcess = batch;
	}

	void Connect() {
		networkCore->Connect();
	}

	void FrameMove();
};

