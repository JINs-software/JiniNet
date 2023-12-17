#pragma once
#include "JBuffer.h"
#include <WinSock2.h>

struct stJNetSession {
	SOCKET sock;
	JBuffer* recvBuff;
	JBuffer* sendBuff;

	stJNetSession(SOCKET sock, UINT recvBuffSize, UINT sendBuffSize) {
		this->sock = sock;
		recvBuff = new JBuffer(recvBuffSize);
		sendBuff = new JBuffer(sendBuffSize);
	}
	~stJNetSession() {
		closesocket(sock);
		delete recvBuff;
		delete sendBuff;
	}
};