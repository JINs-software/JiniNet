#include "JNetServerNetworkCore.h"

static HostID g_HostID = 3;

JNetServerNetworkCore::JNetServerNetworkCore() {
	eventHandler = new JNetServerEventHandler();
	FD_ZERO(&remoteReadSet);
}

bool JNetServerNetworkCore::Start(const stServerStartParam param) {
	//// 논-블로킹 소켓 전환
	//u_long on = 1;
	//if (ioctlsocket(sock, FIONBIO, &on) == SOCKET_ERROR) {
	//	ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::Start ", L"ioctlsocket(..) == SOCKET_ERROR");
	//}
	//// SO_REUSEADDR 옵션 적용
	//int32 optval = 1;
	//if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval)) == SOCKET_ERROR) {
	//	ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::Start ", L"setsocket(..SO_REUSEADDR) == SOCKET_ERROR");
	//}

	int option = TRUE;               //네이글 알고리즘 on/off
	setsockopt(sock,             //해당 소켓
		IPPROTO_TCP,          //소켓의 레벨
		TCP_NODELAY,          //설정 옵션
		(const char*)&option, // 옵션 포인터
		sizeof(option));      //옵션 크기

	SOCKADDR_IN serverAddr = CreateServerADDR(param.IP.c_str(), param.Port);
	BindSocket(sock, serverAddr);
	ListenSocket(sock);
	return true;
}

bool JNetServerNetworkCore::receiveSet() {
	JNetworkCore::receiveSet();

	FD_ZERO(&remoteReadSet);
	bool setFlag = false;
	for (auto iter : remoteMap) {
		const stJNetSession* client = iter.second;
		FD_SET(client->sock, &remoteReadSet);
		setFlag = true;
	}
	if (setFlag) {
		timeval tval = { 0, 0 };
		int retSel = select(0, &remoteReadSet, nullptr, nullptr, &tval);
		if (retSel == SOCKET_ERROR) {
			ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::ReceiveSet()", L"select(..) == SOCKET_ERROR", WSAGetLastError());
			return false;
		}
	}

	return true;
}
bool JNetServerNetworkCore::receive() {
	if (FD_ISSET(sock, &readSet)) {
		if (eventHandler->OnConnectRequest()) {

			SOCKADDR_IN clientAddr;
			SOCKET clientSock = AcceptSocket(sock, clientAddr);
			//remoteSet.insert(clientSock);
			//clientMap.insert({ g_HostID++, new stJNetSession(clientSock, SESSION_RECV_BUFF, SESSION_SEND_BUFF) });
			HostID allocID = g_HostID++;
			remoteMap.insert({ allocID, new stJNetSession(clientSock, SESSION_RECV_BUFF, SESSION_SEND_BUFF) });

			eventHandler->OnClientJoin(allocID);

			//stMSG_HDR msgHdr;
			//msgHdr.msgID = JNET_MSG_ALLOC;
			//msgHdr.msgLen = sizeof(HostID);
			//msgHdr.uniqueNum = JNET_UNIQUE_MSG_NUM;
			//JBuffer buff(sizeof(stMSG_HDR) + msgHdr.msgLen);
			//buff.Enqueue(reinterpret_cast<const BYTE*>(&msgHdr), sizeof(stMSG_HDR));
			//buff.Enqueue(reinterpret_cast<const BYTE*>(&allocID), sizeof(hostID));
			//::send(clientSock, reinterpret_cast<const char*>(buff.GetDequeueBufferVoidPtr()), buff.GetUseSize(), 0);
		}
	}
	// TO DO: 클라이언트 통신 소켓 저장 및 아이디 공유
	// -> JNetServer에서 판단 및 진행

	//for (SOCKET s : remoteSet) {
	for (auto iter : remoteMap) {
		stJNetSession* client = iter.second;
		if (FD_ISSET(client->sock, &remoteReadSet)) {
			stJNetSession* client = iter.second;
			int recvLen = recv(client->sock, reinterpret_cast<char*>(client->recvBuff->GetEnqueueBufferPtr()), client->recvBuff->GetDirectEnqueueSize(), 0);
			//*iptr = recvLen;
			if (recvLen == SOCKET_ERROR) {
				// TO DO: 에러 처리
				if (eventHandler->OnClientDisconnect(iter.first)) {
					// OnClientDisconnect 이벤트를 받고, true를 받환하면 코어 측에서 연결 종료 처리
					Disconnect(iter.first);
				}
			}
			else {
				client->recvBuff->DirectMoveEnqueueOffset(recvLen);

				if (!oneway) {
					stJMSG_HDR hdr;
					client->recvBuff->Peek(&hdr);
					if (stupMap.find(hdr.msgID) != stupMap.end()) {
						stupMap[hdr.msgID]->ProcessReceivedMessage(iter.first, *client->recvBuff);
					}
					else {
						ERROR_EXCEPTION_WINDOW(L"receive()", L"Undefined Message");
					}
				}
				else {
					stupMap[ONEWAY_RPCID]->ProcessReceivedMessage(iter.first, *client->recvBuff);
				}
			}
		}
	}

	return true;
}
bool JNetServerNetworkCore::sendSet() {
	FD_ZERO(&writeSet);
	bool setFlag = false;
	//for (SOCKET s : remoteSet) {
	for (auto iter : remoteMap) {
		stJNetSession* client = iter.second;
		FD_SET(client->sock, &writeSet);
		setFlag = true;
	}
	timeval tval = { 0, 0 };
	if (setFlag) {
		int retSel = select(0, nullptr, &writeSet, nullptr, &tval);
		if (retSel == SOCKET_ERROR) {
			ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::sendSet()", L"select(..) == SOCKET_ERROR", WSAGetLastError());
			return false;
		}
	}
	return true;
}
bool JNetServerNetworkCore::send() {
	for (auto iter : remoteMap) {
		stJNetSession* client = iter.second;
		if (FD_ISSET(client->sock, &writeSet)) {
			while (client->sendBuff->GetUseSize() > 0) {
				int len = client->sendBuff->GetUseSize();
				uint32 dirDeqSize = client->sendBuff->GetDirectDequeueSize();
				int sendLen;
				if (len <= dirDeqSize) {
					sendLen = ::send(client->sock, reinterpret_cast<const char*>(client->sendBuff->GetDequeueBufferPtr()), len, 0);
				}
				else {
					sendLen = ::send(client->sock, reinterpret_cast<const char*>(client->sendBuff->GetDequeueBufferPtr()), dirDeqSize, 0);
				}

				if (sendLen == SOCKET_ERROR) {
					// 에러 처리
					// TO DO: 에러 처리
					if (eventHandler->OnClientDisconnect(iter.first)) {
						// OnClientDisconnect 이벤트를 받고, true를 받환하면 코어 측에서 연결 종료 처리
						Disconnect(iter.first);
					}
					break;
				}
				else {
					client->sendBuff->DirectMoveDequeueOffset(sendLen);

					len -= sendLen;
					if (len == 0) {
						break;
					}
					else if (len > 0) {
						ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::send ", L"sendBufferLen > sendLen");
					}
					else if (len < 0) {
						// EXCEPTION
						ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::send ", L"sendBufferLen < sendLen");
					}
				}
			}
		}
	}

	return true;
}