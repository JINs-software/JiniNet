#pragma once
#include "JBuffer.h"
#include <WinSock2.h>

struct stJNetSession {
	HostID hostID;
	SOCKET sock;
	JBuffer* recvBuff;
	JBuffer* sendBuff;

//#ifdef REMOTE_VEC
	stJNetSession* prevSession;
	stJNetSession* nextSession;
//#endif // REMOTE_VEC

	stJNetSession(SOCKET sock, UINT recvBuffSize, UINT sendBuffSize) {
		this->sock = sock;
		recvBuff = new JBuffer(recvBuffSize);
		sendBuff = new JBuffer(sendBuffSize);

//#ifdef REMOTE_VEC
		prevSession = nullptr;
		nextSession = nullptr;
//#endif // REMOTE_VEC
	}
	~stJNetSession() {
		closesocket(sock);
		delete recvBuff;
		delete sendBuff;
	}
};