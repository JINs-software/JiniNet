#pragma once
#include "SocketUtil.h"
#include "JBuffer.h"
#include "JNetMsgConfig.h"
#include "JNetSession.h"

/*** 관리 메시지 번호 ***/
#define JNET_UNIQUE_MSG_NUM 0x88;
#define JNET_MSG_ALLOC 100

#define SERVER_HOST_ID 0

class JNetworkCore
{
	friend class JNetProxy;
private:
	WSADATA wsaData;
protected:
	std::map<HostID, stJNetSession*> remoteMap;
	std::set<HostID> disconnectedSet;
	SOCKET sock;
protected:
	fd_set readSet;
	fd_set writeSet;

public:
	JNetworkCore() {
		FD_ZERO(&readSet);
		FD_ZERO(&writeSet);
		InitWindowSocketLib(&wsaData);
		sock = CreateWindowSocket_IPv4(true);
	}
	bool Receive() {
		if (!receiveSet()) {
			return false;
		}
		return receive();
	}
	bool Send() {
		return send();
	}

protected:
	virtual bool receiveSet();
	virtual bool receive() = 0;
	virtual bool sendSet() = 0;
	virtual bool send() = 0;

	void setDisconnected(HostID remote) {
		disconnectedSet.insert(remote);
	}
	void clearDisconnected() {
		for (HostID remote : disconnectedSet) {
			if (remoteMap.find(remote) != remoteMap.end()) {
				remoteMap.erase(remote);
				// TO DO: erase 시 stJNetSession 객체가 정상적으로 반환되는가?
			}
		}
		disconnectedSet.clear();
	}

protected:
	void ERROR_EXCEPTION_WINDOW(const WCHAR* wlocation, const WCHAR* wcomment, int errcode = -999999) {
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

};

