#include "JNetServer.h"

JNetServer::JNetServer() {
	networkCore = new JNetServerNetworkCore();
	batchProcess = new JNetBatchProcess();
	recvBuff = new JBuffer(SERV_RECV_BUFF_SIZE);
	sendBuff = new JBuffer(SERV_SEND_BUFF_SIZE);
}

inline void JNetServer::AttachEventHandler(JNetServerEventHandler* eventHandler) {
	networkCore->AttachEventHandler(eventHandler);
}
inline void JNetServer::AttachProxy(JNetProxy* proxy, BYTE unique) {
	proxy->netcore = networkCore;
	proxy->uniqueNum = unique;
}
inline void JNetServer::AttachStub(JNetStub* stub, BYTE unique) {
	networkCore->AttachStub(stub);
	stub->uniqueNum = unique;
}
inline void JNetServer::AttachBatchProcess(JNetBatchProcess* batch) {
	if (!batchProcess) {
		delete batchProcess;
	}
	batchProcess = batch;
}

inline void JNetServer::Start(const stServerStartParam& param) {
	networkCore->Start(param);
}

void JNetServer::FrameMove()
{
	networkCore->Receive();

	// Batch Proccessing..
	batchProcess->BatchProcess();

	networkCore->Send();
}
