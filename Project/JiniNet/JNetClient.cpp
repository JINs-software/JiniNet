#include "JNetClient.h"

JNetClient::JNetClient() {
	networkCore = new JNetClientNetworkCore();
	batchProcess = new JNetBatchProcess();

	recvBuff = new JBuffer(CLIENT_RECV_BUFF_SIZE);
	sendBuff = new JBuffer(CLIENT_SEND_BUFF_SIZE);
}
inline void JNetClient::AttachEventHandler(JNetClientEventHandler* eventHandler) {
	networkCore->AttachEventHandler(eventHandler);
}
inline void JNetClient::AttachProxy(JNetProxy* proxy, BYTE unique) {
	proxy->netcore = networkCore;
	proxy->uniqueNum = unique;
}
inline void JNetClient::AttachStub(JNetStub* stub, BYTE unique) {
	networkCore->AttachStub(stub);
	stub->uniqueNum = unique;
}
inline void JNetClient::AttachBatchProcess(JNetBatchProcess* batch) {
	if (!batchProcess) {
		delete batchProcess;
	}
	batchProcess = batch;
}
inline void JNetClient::Connect() {
	networkCore->Connect();
}

void JNetClient::FrameMove()
{
	networkCore->Receive();

	// Batch Proccessing..
	batchProcess->BatchProcess();

	networkCore->Send();

}
