#include "JNetClient.h"

JNetClient::JNetClient() {
	networkCore = new JNetClientNetworkCore();
	batchProcess = new JNetBatchProcess();

	recvBuff = new JBuffer(CLIENT_RECV_BUFF_SIZE);
	sendBuff = new JBuffer(CLIENT_SEND_BUFF_SIZE);
}

void JNetClient::FrameMove()
{
	networkCore->Receive();

	// Batch Proccessing..
	batchProcess->BatchProcess();

	networkCore->Send();

}
