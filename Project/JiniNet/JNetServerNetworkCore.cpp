#include "JNetServerNetworkCore.h"
#include <cassert>

static HostID g_HostID = 3;

JNetServerNetworkCore::JNetServerNetworkCore() {
	eventHandler = new JNetServerEventHandler();
	
	//FD_ZERO(&remoteReadSet);
	//remoteReadSets.push_back(fd_set());
	//FD_ZERO(&remoteReadSets[0]);
}

bool JNetServerNetworkCore::Start(const stServerStartParam param) {
	//// ��-���ŷ ���� ��ȯ
	u_long on = 1;
	if (ioctlsocket(sock, FIONBIO, &on) == SOCKET_ERROR) {
		ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::Start ", L"ioctlsocket(..) == SOCKET_ERROR");
	}
	// SO_REUSEADDR �ɼ� ����
	int32 optval = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval)) == SOCKET_ERROR) {
		ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::Start ", L"setsocket(..SO_REUSEADDR) == SOCKET_ERROR");
	}
	// ���� ������ ���� ���� ��û�� ���� �����̵�, Ŭ���̾�Ʈ ���� ����/������ ���� ó�� �����̵� 
	// TCP ���� ���Ḧ ����
	// => ��Ŀ 1/0 �ɼ� ���� (TCP ��� ���Ͽ� �����Ͽ� accept �Լ��� ���ϵǴ� ������ �ڵ����� �� �ɼ����� ������
	LINGER lingerOptval;
	lingerOptval.l_onoff = 1;
	lingerOptval.l_linger = 0;
	if (setsockopt(sock, SOL_SOCKET, SO_LINGER, (char*)&lingerOptval, sizeof(lingerOptval)) == SOCKET_ERROR) {
		ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::Start ", L"setsocket(..SO_LINGER) == SOCKET_ERROR");
	}

	//int option = TRUE;               //���̱� �˰��� on/off
	//setsockopt(sock,             //�ش� ����
	//	IPPROTO_TCP,          //������ ����
	//	TCP_NODELAY,          //���� �ɼ�
	//	(const char*)&option, // �ɼ� ������
	//	sizeof(option));      //�ɼ� ũ��

	SOCKADDR_IN serverAddr = CreateServerADDR(param.IP.c_str(), param.Port);
	BindSocket(sock, serverAddr);
	//ListenSocket_SOMAXCONNHINT(sock, 1000);
	if (listen(sock, SOMAXCONN_HINT(1000)) == SOCKET_ERROR) {
		int32 errCode = ::WSAGetLastError();
		cout << "[ERR]: " << errCode << endl;
	}

	return true;
}

bool JNetServerNetworkCore::receiveSet() {
	//JNetworkCore::receiveSet();

	/* Ŭ���̾�Ʈ ��� ���� 64 �ʰ� ���� �߰� */
#ifdef REMOTE_MAP
	auto iter = remoteMap.begin();
#endif // REMOTE_MAP
#ifdef REMOTE_VEC
	stJNetSession* session = sessionMgr.GetSessionFront();
#endif // REMOTE_VEC

	bool endFlag = false;
	for(int sidx = 0; !endFlag; sidx++) {
		if (remoteReadSets.size() <= sidx) {
			remoteReadSets.push_back(fd_set());
		}

		fd_set& remoteReadSet = remoteReadSets[sidx];

		FD_ZERO(&remoteReadSet);
		bool setFlag = false;

		for (int idx = 0; idx < FD_SETSIZE; idx++) {
			const stJNetSession* client = nullptr;
#ifdef REMOTE_MAP
			if (iter == remoteMap.end()) {
				endFlag = true;
				break;
			}
			else {
				client = iter->second;
			}
#endif // DEBUG
#ifdef REMOTE_VEC
			if (session == nullptr) {
				endFlag = true;
				break;
			}
			else {
				client = session;
			}
#endif // REMOTE_VEC

			FD_SET(client->sock, &remoteReadSet);
			setFlag = true;

#ifdef REMOTE_MAP
			iter++;
#endif // REMOTE_MAP
#ifdef REMOTE_VEC
			session = session->nextSession;
#endif // REMOTE_VEC
		}
		if (setFlag) {
			timeval tval = { 0, 0 };
			int retSel = select(0, &remoteReadSet, nullptr, nullptr, &tval);
			if (retSel == SOCKET_ERROR) {
				ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::ReceiveSet()", L"select(..) == SOCKET_ERROR", WSAGetLastError());
				return false;
			}
		}
	}

	return true;
}
bool JNetServerNetworkCore::receive() {
//	if (FD_ISSET(sock, &readSet)) {
//		if (eventHandler->OnConnectRequest()) {
//#ifdef REMOTE_MAP
//			if (g_HostID > HOST_ID_LIMIT) {
//				return false;
//			}
//#endif // REMOTE_MAP
//#ifdef REMOTE_VEC
//			if (sessionMgr.availableID.empty()) {
//				return false;
//			}
//#endif // REMOTE_VEC
//
//			SOCKADDR_IN clientAddr;
//			SOCKET clientSock = AcceptSocket(sock, clientAddr);
//			
//			HostID allocID = 0; 
//
//#ifdef REMOTE_MAP
//			allocID = g_HostID++;
//			remoteMap.insert({ allocID, new stJNetSession(clientSock, SESSION_RECV_BUFF, SESSION_SEND_BUFF) });
//#endif // REMOTE_MAP
//#ifdef REMOTE_VEC
//			sessionMgr.SetSession(clientSock, SESSION_RECV_BUFF, SESSION_SEND_BUFF, allocID);
//#endif // REMOTE_VEC
//
//			if (allocID != 0) {
//				eventHandler->OnClientJoin(allocID);
//			}
//		}
//	}
	// TO DO: Ŭ���̾�Ʈ ��� ���� ���� �� ���̵� ����
	// -> JNetServer���� �Ǵ� �� ����

#ifdef REMOTE_MAP
	auto iter = remoteMap.begin();
#endif // REMOTE_MAP
#ifdef REMOTE_VEC
	stJNetSession* session = sessionMgr.GetSessionFront();
#endif // REMOTE_VEC
	bool endFlag = false;
	for (int sidx = 0; !endFlag; sidx++) {
		/////////////////////////////////////////////////
		//////////// Listen Socket Ȯ�� ////////////////
		// ���� ���� Ȯ�� �۾��� �� ����ϰ� �ϵ��� ��
		JNetworkCore::receiveSet();
		if (FD_ISSET(sock, &readSet)) {
			if (eventHandler->OnConnectRequest()) {
#ifdef REMOTE_MAP
				if (g_HostID > HOST_ID_LIMIT) {
					return false;
				}
#endif // REMOTE_MAP
#ifdef REMOTE_VEC
				if (sessionMgr.availableID.empty()) {
					return false;
				}
#endif // REMOTE_VEC

				SOCKADDR_IN clientAddr;
				SOCKET clientSock = AcceptSocket(sock, clientAddr);

				HostID allocID = 0;

#ifdef REMOTE_MAP
				allocID = g_HostID++;
				remoteMap.insert({ allocID, new stJNetSession(clientSock, SESSION_RECV_BUFF, SESSION_SEND_BUFF) });
#endif // REMOTE_MAP
#ifdef REMOTE_VEC
				sessionMgr.SetSession(clientSock, SESSION_RECV_BUFF, SESSION_SEND_BUFF, allocID);
#endif // REMOTE_VEC

				if (allocID != 0) {
					eventHandler->OnClientJoin(allocID);
				}
			}
		}
		/////////////////////////////////////////////////
		/////////////////////////////////////////////////

		for (int idx = 0; idx < FD_SETSIZE; idx++) {
			const stJNetSession* client = nullptr;
			HostID hID = 0;
#ifdef REMOTE_MAP
			if (iter == remoteMap.end()) {
				endFlag = true;
				break;
			}
			else {
				client = iter->second;
				hID = iter->first;
			}
#endif // DEBUG
#ifdef REMOTE_VEC
			if (session == nullptr) {
				endFlag = true;
				break;
			}
			else {
				client = session;
				hID = session->hostID;
			}
#endif // REMOTE_VEC
			fd_set& remoteReadSet = remoteReadSets[sidx];
			if (FD_ISSET(client->sock, &remoteReadSet)) {
				while (true) {
					// ���̷�Ʈ�� ���� �� �ִ� ��ŭ �� �޴´�. ���� TCP ���� ���ۿ� �����Ͱ� �����ִµ�, ���̷�Ʈ �뷮�� �����ϴٸ� while(true)������ ���� �� �ִ�.
					int recvLen = recv(client->sock, reinterpret_cast<char*>(client->recvBuff->GetEnqueueBufferPtr()), client->recvBuff->GetDirectEnqueueSize(), 0);
					//*iptr = recvLen;
					if (recvLen == SOCKET_ERROR) {
						// TO DO: ���� ó��
						// select ���̱⿡ WSAEWOULDBLOCK ������ �߻����� ������ ����Ѵ�. 
						// ����ڵ� 
						int errCode = WSAGetLastError();
						if (errCode != WSAEWOULDBLOCK) {
							//std::cout << "[JNet::RECV] HostID: " << hID << "WSAGetLastError: " << errCode << std::endl;
							//if (eventHandler->OnClientDisconnect(hID)) {
							//	// OnClientDisconnect �̺�Ʈ�� �ް�, true�� ��ȯ�ϸ� �ھ� ������ ���� ���� ó��
							//	Disconnect(hID);
							//}
							//=> �̹� ���� ��û�� Ŭ���̾�Ʈ�� �ߺ����� ������ �� �� ����
							if (Disconnect(hID)) {
								eventHandler->OnClientDisconnect(hID);
							}
						}
						break;
					}
					else if (recvLen == 0) {
						// ��������� ���� ���� ���� ��û (FIN)
						if (eventHandler->OnClientDisconnect(hID)) {
							// OnClientDisconnect �̺�Ʈ�� �ް�, true�� ��ȯ�ϸ� �ھ� ������ ���� ���� ó��
							Disconnect(hID);
						}
						break;
					}
					else {
						client->recvBuff->DirectMoveEnqueueOffset(recvLen);

						if (!oneway) {
							stJMSG_HDR hdr;
							client->recvBuff->Peek(&hdr);
							if (stupMap.find(hdr.msgID) != stupMap.end()) {
								stupMap[hdr.msgID]->ProcessReceivedMessage(hID, *client->recvBuff);
							}
							else {
								ERROR_EXCEPTION_WINDOW(L"receive()", L"Undefined Message");
							}
						}
						else {
							stupMap[ONEWAY_RPCID]->ProcessReceivedMessage(hID, *client->recvBuff);
						}
					}
				}
			}

#ifdef REMOTE_MAP
			iter++;
#endif // REMOTE_MAP
#ifdef REMOTE_VEC
			session = session->nextSession;
#endif // REMOTE_VEC
		}
	}

	return true;
}
bool JNetServerNetworkCore::sendSet() {
#ifdef REMOTE_MAP
	auto iter = remoteMap.begin();
#endif // REMOTE_MAP
#ifdef REMOTE_VEC
	stJNetSession* session = sessionMgr.GetSessionFront();
#endif // REMOTE_VEC
	bool endFlag = false;
	for (int sidx = 0; !endFlag; sidx++) {
		if (remoteWriteSets.size() <= sidx) {
			remoteWriteSets.push_back(fd_set());
		}
		fd_set& remoteWriteSet = remoteWriteSets[sidx];
		FD_ZERO(&remoteWriteSet);
		bool setFlag = false;

		for (int idx = 0; idx < FD_SETSIZE; idx++) {
			const stJNetSession* client = nullptr;
#ifdef REMOTE_MAP
			if (iter == remoteMap.end()) {
				endFlag = true;
				break;
			}
			else {
				client = iter->second;
			}
#endif // DEBUG
#ifdef REMOTE_VEC
			if (session == nullptr) {
				endFlag = true;
				break;
			}
			else {
				client = session;
			}
#endif // REMOTE_VEC
			FD_SET(client->sock, &remoteWriteSet);
			setFlag = true;
#ifdef REMOTE_MAP
			iter++;
#endif // REMOTE_MAP
#ifdef REMOTE_VEC
			session = session->nextSession;
#endif // REMOTE_VEC
		}
		if (setFlag) {
			timeval tval = { 0, 0 };
			int retSel = select(0, nullptr, &remoteWriteSet, nullptr, &tval);
			if (retSel == SOCKET_ERROR) {
				ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::sendSet()", L"select(..) == SOCKET_ERROR", WSAGetLastError());
				return false;
			}
		}
	}

	return true;
}
bool JNetServerNetworkCore::send() {
#ifdef REMOTE_MAP
	auto iter = remoteMap.begin();
#endif // REMOTE_MAP
#ifdef REMOTE_VEC
	stJNetSession* session = sessionMgr.GetSessionFront();
#endif // REMOTE_VEC
	bool endFlag = false;
	for (int sidx = 0; !endFlag; sidx++) {
		for (int idx = 0; idx < FD_SETSIZE; idx++) {
			const stJNetSession* client = nullptr;
			HostID hID = 0;
#ifdef REMOTE_MAP
			if (iter == remoteMap.end()) {
				endFlag = true;
				break;
			}
			else {
				client = iter->second;
				hID = iter->first;
			}
#endif // DEBUG
#ifdef REMOTE_VEC
			if (session == nullptr) {
				endFlag = true;
				break;
			}
			else {
				client = session;
				hID = session->hostID;
			}
#endif // REMOTE_VEC
			fd_set& remoteWriteSet = remoteWriteSets[sidx];
			if (FD_ISSET(client->sock, &remoteWriteSet)) {
				while (client->sendBuff->GetUseSize() > 0) {
					int len = client->sendBuff->GetUseSize();
					uint32 dirDeqSize = client->sendBuff->GetDirectDequeueSize();
					int sendLen;
					if (len == dirDeqSize) {
						sendLen = ::send(client->sock, reinterpret_cast<const char*>(client->sendBuff->GetDequeueBufferPtr()), len, 0);
					}
					else if(len > dirDeqSize) {
						sendLen = ::send(client->sock, reinterpret_cast<const char*>(client->sendBuff->GetDequeueBufferPtr()), dirDeqSize, 0);
					}
					else {
						cout << "�ǵ����� ���� �帧: sendBuff->GetUseSize < sendBuff->dirDeqSize" << endl;
						assert(false);
					}

					if (sendLen == SOCKET_ERROR) {
						// ���� ó��
						// TO DO: ���� ó��
						// select ���̱⿡ WSAEWOULDBLOCK ������ �߻����� ������ ����Ѵ�. 
						// ����ڵ� 
						int errCode = WSAGetLastError();
						if (errCode != WSAEWOULDBLOCK) {
							//std::cout << "[JNet::SEND] HostID: " << hID << "WSAGetLastError: " << errCode << std::endl;
							//if (eventHandler->OnClientDisconnect(hID)) {
							//	// OnClientDisconnect �̺�Ʈ�� �ް�, true�� ��ȯ�ϸ� �ھ� ������ ���� ���� ó��
							//	Disconnect(hID);
							//}
							if (Disconnect(hID)) {
								eventHandler->OnClientDisconnect(hID);
							}
						}
						break;
					}
					else {
						if (sendLen < len) {
							std::cout << "[SEND] sendLen < len : TCP �۽� ���� ���� ����" << std::endl;
							ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::send", L"sendLen < len : TCP �۽� ���� ���� ����");
						}

						client->sendBuff->DirectMoveDequeueOffset(sendLen);

						len -= sendLen;
						if (len == 0) {
							break;
						}
						//else if (len > 0) {
						//	ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::send ", L"sendBufferLen > sendLen");
						//}
						// => len > dirDeqSize��� ������ �б� �帧��.
						else if (len < 0) {
							// EXCEPTION
							ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::send ", L"sendBufferLen < sendLen");
						}
					}
				}
			}
#ifdef REMOTE_MAP
			iter++;
#endif // REMOTE_MAP
#ifdef REMOTE_VEC
			session = session->nextSession;
#endif // REMOTE_VEC
		}
	}

	return true;
}