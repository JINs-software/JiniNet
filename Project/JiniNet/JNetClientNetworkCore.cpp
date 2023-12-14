#include "JNetClientNetworkCore.h"

JNetClientNetworkCore::JNetClientNetworkCore() {
	eventHandler = new JNetClientEventHandler();
	session = new stJNetSession(sock, CLIENT_RECV_BUFF, CLIENT_SEND_BUFF);
	remoteMap.insert({ SERVER_HOST_ID , session });
}
inline void JNetClientNetworkCore::AttachEventHandler(JNetClientEventHandler* eventHandler) {
	this->eventHandler = eventHandler;
}
inline void JNetClientNetworkCore::AttachStub(JNetStub* stub) {
	RpcID* rpcList = stub->GetRpcList();
	int rpcListCnt = stub->GetRpcListCount();
	for (int i = 0; i < rpcListCnt; i++) {
		stupMap.insert({ rpcList[i], stub });
	}
}

bool JNetClientNetworkCore::Connect() {
	SOCKADDR_IN serverAddr = CreateServerADDR();
	ConnectSocket(sock, serverAddr);

	return true;
}

bool JNetClientNetworkCore::receive() {
	if (FD_ISSET(session->sock, &readSet)) {
		if (session->recvBuff->GetUseSize() <= 0) {
			return false;
		}

		int recvLen = recv(session->sock, reinterpret_cast<char*>(session->recvBuff->GetEnqueueBufferPtr()), session->recvBuff->GetDirectEnqueueSize(), 0);
		if (recvLen == SOCKET_ERROR) {
			// TO DO: recv 俊矾 贸府
		}
		else {
			session->recvBuff->DirectMoveEnqueueOffset(recvLen);

			//stub->ProcessReceivedMessage(SERVER_HOST_ID, *session->recvBuff);
			stMSG_HDR hdr;
			session->recvBuff->Peek(&hdr);
			if (stupMap.find(hdr.msgID) != stupMap.end()) {
				stupMap[hdr.msgID]->ProcessReceivedMessage(SERVER_HOST_ID, *session->recvBuff);
			}
			else {
				ERROR_EXCEPTION_WINDOW(L"receive()", L"Undefined Message");
			}
		}
	}

	return true;
}
bool JNetClientNetworkCore::sendSet() {
	FD_ZERO(&writeSet);
	FD_SET(session->sock, &writeSet);
	timeval tval = { 0, 0 };
	int retSel = select(0, nullptr, &writeSet, nullptr, &tval);
	if (retSel == SOCKET_ERROR) {
		ERROR_EXCEPTION_WINDOW(L"JNetClientNetworkCore::sendSet()", L"select(..) == SOCKET_ERROR", WSAGetLastError());
		return false;
	}
	return true;
}
bool JNetClientNetworkCore::send() {
	if (FD_ISSET(session->sock, &writeSet) && (session->sendBuff->GetUseSize() > 0)) {
		while (session->sendBuff->GetUseSize() > 0) {
			int len = session->sendBuff->GetUseSize();
			uint32 dirDeqSize = session->sendBuff->GetDirectDequeueSize();
			int sendLen;
			if (len <= dirDeqSize) {
				sendLen = ::send(sock, reinterpret_cast<const char*>(session->sendBuff->GetDequeueBufferPtr()), len, 0);
			}
			else {
				sendLen = ::send(sock, reinterpret_cast<const char*>(session->sendBuff->GetDequeueBufferPtr()), dirDeqSize, 0);
			}
			if (sendLen == SOCKET_ERROR) {
				// 俊矾 贸府
			}

			len -= sendLen;
			if (len == 0) {
				break;
			}
			else if (len < 0) {
				// EXCEPTION
			}
		}
	}
	return true;
}