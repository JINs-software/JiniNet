#include "JNetServer.h"

using namespace std;

JNetServer::JNetServer(bool interactive) {
	m_NetworkCore = new JNetCoreServer(!interactive);
	m_BatchProcess = new JNetBatchProcess();
}

void JNetServer::Start(const stServerStartParam& param, long msecPerFrame)
{
	m_NetworkCore->Start(param);

	timeBeginPeriod(1);
	
	uint16	frameDelta;
	clock_t timeStamp = clock();
	clock_t accumulation = 0;
	while (true) {
		clock_t timeNow = clock();
		clock_t timeDuration = timeNow - timeStamp;
		timeStamp = timeNow;
		accumulation += timeDuration;
		frameDelta = accumulation / msecPerFrame;
		accumulation %= msecPerFrame;
		FrameMove(frameDelta);
	}

	timeEndPeriod(1);
}

void JNetServer::FrameMove(uint16 frameDelta) {
#if defined(_DEBUG)
	static time_t logLap = 0;
#endif

	m_NetworkCore->Receive();

	// Batch Proccessing..
	m_BatchProcess->BatchProcess(frameDelta);

	m_NetworkCore->Send();
	
#if defined(_DEBUG)
	if (clock() - logLap > CONSOLE_PRINT_LOG_CYCLE_PARAM) {
		ConsolePrintLog();
		batchProcess->BatchConsoleLog();
		batchProcess->BatchValidCheck();
		logLap = clock();
	}
#endif

}
