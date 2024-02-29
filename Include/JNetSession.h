#pragma once
#include "JBuffer.h"
#include <WinSock2.h>

#include "JNetSessionConfig.h"

struct stJNetSession {
	HostID hostID;
	SOCKET sock;
	//JBuffer* recvBuff;
	//JBuffer* sendBuff;
	// => JNetPool에서 세션 공간을 할당받는데, 내부에서 링퍼버를 new를 통한 동적 할당을 받는다면 풀을 사용하는 목적이 무의미하다.
	BYTE recvBuff_internal[SESSION_RECV_BUFF];
	BYTE sendBuff_internal[SESSION_SEND_BUFF];
	JBuffer recvBuff;
	JBuffer sendBuff;

//#ifdef REMOTE_VEC
	stJNetSession* prevSession = nullptr;
	stJNetSession* nextSession = nullptr;
//#endif // REMOTE_VEC

	stJNetSession(SOCKET _sock, HostID _hostID) 
		: sock(_sock), hostID(_hostID), recvBuff(SESSION_RECV_BUFF, recvBuff_internal), sendBuff(SESSION_SEND_BUFF, sendBuff_internal)
	{}
	~stJNetSession() {
		closesocket(sock);
	}

//	stJNetSession(SOCKET sock, UINT recvBuffSize, UINT sendBuffSize) 
//		: recvBuff(recvBuffSize), sendBuff(sendBuffSize)
//	{
//		this->sock = sock;
//		recvBuff = new JBuffer(recvBuffSize);
//		sendBuff = new JBuffer(sendBuffSize);
//
////#ifdef REMOTE_VEC
//		prevSession = nullptr;
//		nextSession = nullptr;
////#endif // REMOTE_VEC
//	}
//	~stJNetSession() {
//		closesocket(sock);
//		delete recvBuff;
//		delete sendBuff;
//	}

};