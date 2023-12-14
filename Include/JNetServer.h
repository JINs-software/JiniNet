#pragma once
#include "JNetServerNetworkCore.h"
#include "JNetServerEventHandler.h"
#include "JNetProxy.h"
#include "JNetStub.h"
#include "JNetMsgConfig.h"
#include "JNetSession.h"
#include "JNetBatchProcess.h"

#define SERV_RECV_BUFF_SIZE 20'000
#define SERV_SEND_BUFF_SIZE 20'000

class JNetServer
{
private:
	JNetServerNetworkCore* networkCore;
	JNetBatchProcess* batchProcess;
	JBuffer* recvBuff;
	JBuffer* sendBuff;

	std::map<HostID, stJNetSession> sessions;

public:
	JNetServer();

	inline void AttachEventHandler(JNetServerEventHandler* eventHandler);
	inline void AttachProxy(JNetProxy* proxy, BYTE unique);
	inline void AttachStub(JNetStub* stub, BYTE unique);
	inline void AttachBatchProcess(JNetBatchProcess* batch);
	inline void Start(const stServerStartParam& param);
	void FrameMove();

private:

};

