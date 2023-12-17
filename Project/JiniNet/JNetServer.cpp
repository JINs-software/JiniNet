#include "JNetServer.h"

JNetServer::JNetServer(bool interactive) {
	networkCore = new JNetServerNetworkCore();
	if (!interactive) {
		networkCore->SetOneway();
	}
	batchProcess = new JNetBatchProcess();
	recvBuff = new JBuffer(SERV_RECV_BUFF_SIZE);
	sendBuff = new JBuffer(SERV_SEND_BUFF_SIZE);
}

void JNetServer::FrameMove()
{
	networkCore->Receive();

	// Batch Proccessing..
	batchProcess->BatchProcess();

	networkCore->Send();
}
