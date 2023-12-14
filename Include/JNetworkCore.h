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
	JNetworkCore();
	inline bool Receive() {
		if (!receiveSet()) {
			return false;
		}
		return receive();
	}
	inline bool Send() {
		return send();
	}

protected:
	virtual bool receiveSet();
	virtual bool receive() = 0;
	virtual bool sendSet() = 0;
	virtual bool send() = 0;

	inline void setDisconnected(HostID remote) {
		disconnectedSet.insert(remote);
	}
	void clearDisconnected();

	void ERROR_EXCEPTION_WINDOW(const WCHAR* wlocation, const WCHAR* wcomment, int errcode = -999999);

};

