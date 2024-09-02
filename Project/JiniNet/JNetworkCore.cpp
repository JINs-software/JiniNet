#include "JNetworkCore.h"

JNetworkCore::JNetworkCore() {
	//FD_ZERO(&readSet);
	//FD_ZERO(&writeSet);
	//InitWindowSocketLib(&wsaData);
	//m_Socket = CreateWindowSocket_IPv4(true);

	InitWindowSocketLib(&wsaData);

}

JNetworkCore::~JNetworkCore()
{
	CleanUpWindowSocketLib();
}

void JNetworkCore::Start(long msecPerFrame)
{
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

void JNetworkCore::FrameMove(uint16 frameDelta)
{
	Receive();
	m_BatchProcess->BatchProcess(frameDelta);
	Send();
}

/*
bool JNetworkCore::receiveSet()
{
	FD_ZERO(&readSet);
	FD_SET(m_Socket, &readSet);
	timeval tval = { 0, 0 };
	int retSel = select(0, &readSet, nullptr, nullptr, &tval);
	if (retSel == SOCKET_ERROR) {
		ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::ReceiveSet()", L"select(..) == SOCKET_ERROR", WSAGetLastError());
		return false;
	}

	return true;
}
bool JNetworkCore::sendSet() {
	return false;
}

void JNetworkCore::batchDisconnection() {
	// 제한 변수 초기화
	deleteCnt = 0;

	for (HostID remote : disconnectedSet) {
		m_SessionMgr.DeleteSession(remote);
	}
	disconnectedSet.clear();

	for (HostID remote : forcedDisconnectedSet) {
		m_SessionMgr.DeleteSession(remote);
	}
	forcedDisconnectedSet.clear();
}
*/

void JNetworkCore::ERROR_EXCEPTION_WINDOW(const WCHAR* wlocation, const WCHAR* wcomment, int errcode) {
	std::wstring wstr = wlocation;
	wstr += L"\n";
	wstr += wcomment;
	wstr += L"\n";
	if (errcode != -999999) {
		wstr += L"errcode: ";
		wstr += errcode;
		wstr += L"\n";
	}
	MessageBox(NULL, wcomment, wstr.c_str(), MB_OK | MB_ICONWARNING);
}
