#include "JNetServerNetworkCore.h"

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

	int option = TRUE;               //���̱� �˰��� on/off
	setsockopt(sock,             //�ش� ����
		IPPROTO_TCP,          //������ ����
		TCP_NODELAY,          //���� �ɼ�
		(const char*)&option, // �ɼ� ������
		sizeof(option));      //�ɼ� ũ��

	SOCKADDR_IN serverAddr = CreateServerADDR(param.IP.c_str(), param.Port);
	BindSocket(sock, serverAddr);
	ListenSocket(sock);
	return true;
}

bool JNetServerNetworkCore::receiveSet() {
	JNetworkCore::receiveSet();

	/* Ŭ���̾�Ʈ ��� ���� 64 �ʰ� ���� �߰� */
	auto iter = remoteMap.begin();
	bool endFlag = false;
	for(int sidx = 0; !endFlag; sidx++) {
		if (remoteReadSets.size() <= sidx) {
			remoteReadSets.push_back(fd_set());
		}
		FD_ZERO(&remoteReadSets[sidx]);
		bool setFlag = false;

		for (int idx = 0; idx < FD_SETSIZE; idx++) {
			if (iter == remoteMap.end()) {
				endFlag = true;
				break;
			}

			const stJNetSession* client = iter->second;
			FD_SET(client->sock, &remoteReadSets[sidx]);
			setFlag = true;
			iter++;
		}
		if (setFlag) {
			timeval tval = { 0, 0 };
			int retSel = select(0, &remoteReadSets[sidx], nullptr, nullptr, &tval);
			if (retSel == SOCKET_ERROR) {
				ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::ReceiveSet()", L"select(..) == SOCKET_ERROR", WSAGetLastError());
				return false;
			}
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
	// TO DO: Ŭ���̾�Ʈ ��� ���� ���� �� ���̵� ����
	// -> JNetServer���� �Ǵ� �� ����

	auto iter = remoteMap.begin();
	bool endFlag = false;
	for (int sidx = 0; !endFlag; sidx++) {
		for (int idx = 0; idx < FD_SETSIZE; idx++) {
			if (iter == remoteMap.end()) {
				endFlag = true;
				break;
			}

			stJNetSession* client = iter->second;
			if (FD_ISSET(client->sock, &remoteReadSets[sidx])) {
				int recvLen = recv(client->sock, reinterpret_cast<char*>(client->recvBuff->GetEnqueueBufferPtr()), client->recvBuff->GetDirectEnqueueSize(), 0);
				//*iptr = recvLen;
				if (recvLen == SOCKET_ERROR) {
					// TO DO: ���� ó��
					// select ���̱⿡ WSAEWOULDBLOCK ������ �߻����� ������ ����Ѵ�. 
					// ����ڵ� 
					if (WSAGetLastError() != WSAEWOULDBLOCK) {
						if (eventHandler->OnClientDisconnect(iter->first)) {
							// OnClientDisconnect �̺�Ʈ�� �ް�, true�� ��ȯ�ϸ� �ھ� ������ ���� ���� ó��
							Disconnect(iter->first);
						}
					}
				}
				else if (recvLen == 0) {
					// ��������� ���� ���� ���� ��û (FIN)
					if (eventHandler->OnClientDisconnect(iter->first)) {
						// OnClientDisconnect �̺�Ʈ�� �ް�, true�� ��ȯ�ϸ� �ھ� ������ ���� ���� ó��
						Disconnect(iter->first);
					}
				}
				else {
					client->recvBuff->DirectMoveEnqueueOffset(recvLen);

					if (!oneway) {
						stJMSG_HDR hdr;
						client->recvBuff->Peek(&hdr);
						if (stupMap.find(hdr.msgID) != stupMap.end()) {
							stupMap[hdr.msgID]->ProcessReceivedMessage(iter->first, *client->recvBuff);
						}
						else {
							ERROR_EXCEPTION_WINDOW(L"receive()", L"Undefined Message");
						}
					}
					else {
						stupMap[ONEWAY_RPCID]->ProcessReceivedMessage(iter->first, *client->recvBuff);
					}
				}
			}

			iter++;
		}
	}

	return true;
}
bool JNetServerNetworkCore::sendSet() {
	auto iter = remoteMap.begin();
	bool endFlag = false;
	for (int sidx = 0; !endFlag; sidx++) {
		if (remoteWriteSets.size() <= sidx) {
			remoteWriteSets.push_back(fd_set());
		}
		FD_ZERO(&remoteWriteSets[sidx]);
		bool setFlag = false;

		for (int idx = 0; idx < FD_SETSIZE; idx++) {
			if (iter == remoteMap.end()) {
				endFlag = true;
				break;
			}

			const stJNetSession* client = iter->second;
			FD_SET(client->sock, &remoteWriteSets[sidx]);
			setFlag = true;
			iter++;
		}
		if (setFlag) {
			timeval tval = { 0, 0 };
			int retSel = select(0, nullptr, &remoteWriteSets[sidx], nullptr, &tval);
			if (retSel == SOCKET_ERROR) {
				ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::sendSet()", L"select(..) == SOCKET_ERROR", WSAGetLastError());
				return false;
			}
		}
	}

	return true;
}
bool JNetServerNetworkCore::send() {
	auto iter = remoteMap.begin();
	bool endFlag = false;
	for (int sidx = 0; !endFlag; sidx++) {
		for (int idx = 0; idx < FD_SETSIZE; idx++) {
			if (iter == remoteMap.end()) {
				endFlag = true;
				break;
			}

			stJNetSession* client = iter->second;
			if (FD_ISSET(client->sock, &remoteWriteSets[sidx])) {
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
						// ���� ó��
						// TO DO: ���� ó��
						// select ���̱⿡ WSAEWOULDBLOCK ������ �߻����� ������ ����Ѵ�. 
						// ����ڵ� 
						if (WSAGetLastError() != WSAEWOULDBLOCK) {
							if (eventHandler->OnClientDisconnect(iter->first)) {
								// OnClientDisconnect �̺�Ʈ�� �ް�, true�� ��ȯ�ϸ� �ھ� ������ ���� ���� ó��
								Disconnect(iter->first);
							}
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

			iter++;
		}
	}

	return true;
}