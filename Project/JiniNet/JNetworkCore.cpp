#include "JNetworkCore.h"

JNetworkCore::JNetworkCore() {
	FD_ZERO(&readSet);
	FD_ZERO(&writeSet);
	InitWindowSocketLib(&wsaData);
	sock = CreateWindowSocket_IPv4(true);

#ifdef REMOTE_VEC
	remoteVec.resize(10000, nullptr);
#endif // REMOTE_VEC
}

bool JNetworkCore::receiveSet()
{
	FD_ZERO(&readSet);
	FD_SET(sock, &readSet);
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
	for (HostID remote : disconnectedSet) {
		if (remoteMap.find(remote) != remoteMap.end()) {
			delete remoteMap[remote];
			remoteMap.erase(remote);
			// TO DO: erase 시 stJNetSession 객체가 정상적으로 반환되는가?
		}
	}
	disconnectedSet.clear();
	
	for (HostID remote : forcedDisconnectedSet) {
		if (remoteMap.find(remote) != remoteMap.end()) {
			delete remoteMap[remote];
			remoteMap.erase(remote);
			// TO DO: erase 시 stJNetSession 객체가 정상적으로 반환되는가?
		}
	}
	forcedDisconnectedSet.clear();
}

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