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
		//return receive();
		bool ret = receive();
		batchDisconnection();
		return ret;
	}
	inline bool Send() {
		if (!sendSet()) {
			return false;
		}
		//return send();
		bool ret = send();
		batchDisconnection();
		return ret;
	}
	inline bool Disconnect(HostID remote) {
		if (disconnectedSet.find(remote) == disconnectedSet.end()) {
			disconnectedSet.insert(remote);
			return true;
		}
		else {
			return false;
		}
	}

protected:
	virtual bool receiveSet();
	virtual bool receive() = 0;
	virtual bool sendSet();
	virtual bool send() = 0;

	void batchDisconnection();

	void ERROR_EXCEPTION_WINDOW(const WCHAR* wlocation, const WCHAR* wcomment, int errcode = -999999);

};

