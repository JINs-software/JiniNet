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
void JNetServer::FrameMove(uint16 loopDelta) {
#if defined(_DEBUG)
	static time_t logLap = 0;
#endif

	networkCore->Receive();

	// Batch Proccessing..
	batchProcess->BatchProcess(loopDelta);

	networkCore->Send();
	
#if defined(_DEBUG)
	if (clock() - logLap > CONSOLE_PRINT_LOG_CYCLE_PARAM) {
		ConsolePrintLog();
		batchProcess->BatchConsoleLog();
		batchProcess->BatchValidCheck();
		logLap = clock();
	}
#endif

}

void JNetServer::ConsolePrintLog() {
	cout << "JNetSession count: " << this->networkCore->GetConnectedSessionCnt() << endl;
}