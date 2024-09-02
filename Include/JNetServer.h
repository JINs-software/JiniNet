#pragma once
#include "JNetServerNetworkCore.h"
#include "JNetServerEventHandler.h"
#include "JNetProxy.h"
#include "JNetStub.h"
#include "JNetSession.h"
#include "JNetBatchProcess.h"

#define SERV_RECV_BUFF_SIZE 1'000
#define SERV_SEND_BUFF_SIZE 1'000

class JNetServer
{
private:
	JNetCoreServer*		m_NetworkCore;
	JNetBatchProcess*		m_BatchProcess;

	std::map<HostID, stJNetSession> m_Sessions;

public:
	JNetServer(bool interactive);

	inline void AttachEventHandler(JNetServerEventHandler* eventHandler) {
		m_NetworkCore->AttachEventHandler(eventHandler);
	}
	inline void AttachProxy(JNetProxy* proxy, BYTE unique) {
		proxy->netcore = m_NetworkCore;
		proxy->uniqueNum = unique;
	}
	inline void AttachStub(JNetStub* stub, BYTE unique) {
		m_NetworkCore->AttachStub(stub);
		stub->uniqueNum = unique;
	}
	inline void AttachBatchProcess(JNetBatchProcess* batch) {
		if (!m_BatchProcess) {
			delete m_BatchProcess;
		}
		m_BatchProcess = batch;
	}

	void Start(const stServerStartParam& param, long msecPerFrame);

private:
	void FrameMove(uint16 frameDelta);

	void ConsolePrintLog();

private:

};

