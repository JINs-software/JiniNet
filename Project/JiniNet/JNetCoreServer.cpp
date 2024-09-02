#include "JNetCoreServer.h"
#include <cassert>

using namespace std;

static HostID g_HostID = 3;

void JNetCoreServer::AttachEventHandler(JNetServerEventHandler* eventHandler) {
	this->m_EventHandler = eventHandler;
}
void JNetCoreServer::AttachProxy(JNetProxy* proxy, BYTE unique) {
	proxy->netcore = this;
	proxy->uniqueNum = unique;
}
void JNetCoreServer::AttachStub(JNetStub* stub, BYTE unique) {
	RpcID* rpcList = stub->GetRpcList();
	int rpcListCnt = stub->GetRpcListCount();
	for (int i = 0; i < rpcListCnt; i++) {
		m_StupMap.insert({ rpcList[i], stub });
	}
	stub->uniqueNum = unique;
}
void JNetCoreServer::AttachBatchProcess(JNetBatchProcess* batch) {
	//if (!m_BatchProcess) {
	//	delete m_BatchProcess;
	//}
	m_BatchProcess = batch;
}

bool JNetCoreServer::Init(uint16 port, std::string IP)
{
	// ���� ���� �� Accept fd set �ʱ�ȭ
	m_ListenSocket = CreateWindowSocket_IPv4(true);
	FD_ZERO(&m_AcceptSet);

	// ��-���ŷ ���� ��ȯ
	u_long on = 1;
	if (ioctlsocket(m_ListenSocket, FIONBIO, &on) == SOCKET_ERROR) {
		ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::Start ", L"ioctlsocket(..) == SOCKET_ERROR");
	}

	// SO_REUSEADDR �ɼ� ����
	int32 optval = 1;
	if (setsockopt(m_ListenSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval)) == SOCKET_ERROR) {
		ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::Start ", L"setsocket(..SO_REUSEADDR) == SOCKET_ERROR");
	}

	// ���� ������ ���� ���� ��û�� ���� �����̵�, Ŭ���̾�Ʈ ���� ����/������ ���� ó�� �����̵� 
	// TCP ���� ���Ḧ ����
	// => ��Ŀ 1/0 �ɼ� ���� (TCP ��� ���Ͽ� �����Ͽ� accept �Լ��� ���ϵǴ� ������ �ڵ����� �� �ɼ����� ������
	LINGER lingerOptval;
	lingerOptval.l_onoff = 1;
	lingerOptval.l_linger = 0;
	if (setsockopt(m_ListenSocket, SOL_SOCKET, SO_LINGER, (char*)&lingerOptval, sizeof(lingerOptval)) == SOCKET_ERROR) {
		ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::Start ", L"setsocket(..SO_LINGER) == SOCKET_ERROR");
	}

	// IP ���� ���
	SOCKADDR_IN serverAddr = CreateServerADDR(port);

	BindSocket(m_ListenSocket, serverAddr);
	
	if (listen(m_ListenSocket, SOMAXCONN_HINT(1000)) == SOCKET_ERROR) {
		int32 errCode = ::WSAGetLastError();
		cout << "[ERR]: " << errCode << endl;
	}

	return true;
}

stJNetSession* JNetCoreServer::GetJNetSession(HostID hostID)
{
	return m_SessionManager.GetSession(hostID);
}

void JNetCoreServer::RequestDisconnection(HostID hostID)
{
	registDisconnection(hostID);
}

void JNetCoreServer::Receive()
{
	receiveSet();
	receive();
	cleanUpSession();
}

void JNetCoreServer::Send()
{
	sendSet();
	send();
	cleanUpSession();
}


void JNetCoreServer::receiveSet() {
	// ���� ���� ����� receive �Լ����� Ŭ���̾�Ʈ select �������� �����Ѵ�.
	// �� ����ϰ� ���� ��û�� Ȯ���ϱ� ���ؼ��̴�.
	//JNetworkCore::receiveSet();

	/* Ŭ���̾�Ʈ ��� ���� 64 �ʰ� ���� �߰� */
	stJNetSession* session = m_SessionManager.GetSessionFront();

	bool endFlag = false;
	for (int sidx = 0; !endFlag; sidx++) {
		if (m_RemoteReadSets.size() <= sidx) {
			m_RemoteReadSets.push_back(fd_set());
		}
		fd_set& remoteReadSet = m_RemoteReadSets[sidx];
		FD_ZERO(&remoteReadSet);
		bool setFlag = false;

		for (int idx = 0; idx < FD_SETSIZE; idx++) {
			const stJNetSession* client = nullptr;
			if (session == nullptr) {
				endFlag = true;
				break;
			}
			else {
				client = session;
			}
			FD_SET(client->sock, &remoteReadSet);
			setFlag = true;
			session = session->nextSession;
		}
		if (setFlag) {
			timeval tval = { 0, 0 };
			int retSel = select(0, &remoteReadSet, nullptr, nullptr, &tval);
			if (retSel == SOCKET_ERROR) {
				ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::ReceiveSet()", L"select(..) == SOCKET_ERROR", WSAGetLastError());
				DebugBreak();
			}
		}
	}
}

void JNetCoreServer::receive() {
	stJNetSession* session = m_SessionManager.GetSessionFront();
	bool endFlag = false;
	for (int sidx = 0; !endFlag; sidx++) {
		///////////////////////////////////////////////////////////////////
		// Listen Socket Ȯ�� (���� ���� Ȯ�� �۾��� �� ����ϰ� �ϵ��� ��)
		///////////////////////////////////////////////////////////////////
		FD_ZERO(&m_AcceptSet);
		FD_SET(m_ListenSocket, &m_AcceptSet);
		timeval tval = { 0, 0 };
		int retSel = select(0, &m_AcceptSet, nullptr, nullptr, &tval);
		if (retSel == SOCKET_ERROR) {
			ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::ReceiveSet()", L"select(..) == SOCKET_ERROR", WSAGetLastError());
			DebugBreak();
		}
		if (FD_ISSET(m_ListenSocket, &m_AcceptSet)) {
			if (m_EventHandler->OnConnectRequest()) {
				if (m_SessionManager.SessionAllocable()) {
					SOCKADDR_IN clientAddr;
					SOCKET clientSock = AcceptSocket(m_ListenSocket, clientAddr);

					HostID allocID = 0;
					if (m_SessionManager.SetSession(clientSock, SESSION_RECV_BUFF, SESSION_SEND_BUFF, allocID)) {
						m_EventHandler->OnClientJoin(allocID);
					}
				}
			}
		}

		//////////////////////////////////////////////////////////////////
		// 64�� ������ Ŭ���̾�Ʈ ���� ���� Ȯ�� 
		//////////////////////////////////////////////////////////////////
		for (int idx = 0; idx < FD_SETSIZE; idx++) {
			stJNetSession* client = nullptr;
			HostID hID = 0;

			if (session == nullptr) {
				endFlag = true;
				break;
			}
			else {
				client = session;
				hID = session->hostID;
			}

			fd_set& remoteReadSet = m_RemoteReadSets[sidx];
			if (FD_ISSET(client->sock, &remoteReadSet)) {	// client->sock�� ��-���ŷ ����� ����
				while (true) {
					// ���̷�Ʈ�� ���� �� �ִ� ��ŭ �� �޴´�. ���� TCP ���� ���ۿ� �����Ͱ� �����ִµ�, ���̷�Ʈ �뷮�� �����ϴٸ� while(true)������ ���� �� �ִ�.
					// �׷��� 'JBUFF_DIRPTR_MANUAL_RESET'�� �������� ������ JBuffer�� ��ť, ��ť �����Ͱ� ������ �� �ٽ� �� �����͸� �ٽ� �޸� ���� ���������� �̵���Ų��.
					// �̿� ���̷�Ʈ �뷮�� ���������� ��찡 �幰������ ����ȴ�.
					int recvLen = recv(client->sock, reinterpret_cast<char*>(client->recvBuff.GetEnqueueBufferPtr()), client->recvBuff.GetDirectEnqueueSize(), 0);

					if (recvLen == SOCKET_ERROR) {
						// TO DO: ���� ó��
						// ���� select ���̱⿡ WSAEWOULDBLOCK ������ �߻����� ������ ����Ѵ�. 
						// ������ FD_ISSET���� TRUE�� ��ȯ�ǰ�, ���� �۾��� �� ��, ���̷�Ʈ ��ť ������� �ޱ⿡ ���� �� �ִ� �����ͺ��� �� ���� ���� �� �ִ� ���ɼ��� �ִ�. 
						// ���� while ������ ���� �߰����� ������ �õ��Ѵ�. �̷��� ��� WSAWOULDBLOCK ������ �߻��� �� ������ ����ؾ� �Ѵ�. 

						int errCode = WSAGetLastError();
						if (errCode != WSAEWOULDBLOCK) {
							//std::cout << "[JNet::RECV] HostID: " << hID << "WSAGetLastError: " << errCode << std::endl;
							//if (eventHandler->OnClientDisconnect(hID)) {
							//	// OnClientDisconnect �̺�Ʈ�� �ް�, true�� ��ȯ�ϸ� �ھ� ������ ���� ���� ó��
							//	Disconnect(hID);
							//}
							//=> �̹� ���� ��û�� Ŭ���̾�Ʈ�� �ߺ����� ������ �� �� ����

							// ���� ������ 1�������� �̺�Ʈ �ڵ鷯�� ���� ������ �ܿ��� �����ϵ��� ��û�ϵ��� �Ͽ���.
							//		(������)
							//		-> �������� DeleteFighter �Լ� ȣ��, g_DeleteClientSet<hostID, netCoreSideFlag>.insert(..);
							//		-> g_DeleteClientSet�� ���Ե� ���� ��� ID�鿡 ���� BatchProcess���� �ϰ������� ������
							//			(FrameMove)
							//			- Receive
							//				- ���� ��� ���� �ϰ� ����
							//			- BatchProcess	// BatchDelete
							//			- Send
							//				- ���� ��� ���� �ϰ� ����
							// �׷��� �м��غ��� ���� ���� ������ ���� DeleteFigther�� ��ǻ� ���� ��� �÷��̾� �¿� �߰��ϴ� ���̰�,
							// BatchProcess �ܰ迡�� ���� �÷��̾� �¿� ���� �ϰ������� �����Ѵ�. 
							// �ᱹ ���̺귯�� ���� �� ������ ���� �Ͼ�ٴ� ���̴�. 
							// 
							// BatchDelete������ ���� ��� �ֺ��� �÷��̾�鿡�� DEL_CHARACTER �޽����� �ϰ������� �����ϰ� �Ǵµ�, 
							// �ֺ� �÷��̾���� �����۸� ���� �����Ͽ� ���������� �޽����� �����Ų��. 
							// ������ �ֺ� �÷��̾ �����ϴµ� �־� GetJNetSession �Լ��� ȣ���ϴµ�, �̹� ������ ������ �ִٸ� NULL�� ��ȯ�ϰ� �ȴ�.
							// ������ �÷��̾� ���� -> ���� ���� ������ �ڵ忴�⿡ NULL ��ȯ�� �ǽɽ������µ�, ��ǻ� ��¿ �� ���� NULL�� ��ȯ�Ǵ� ��Ȳ�̴�.
							// �������� ������ �� ������ ���� �Ͼ���� �ڵ带 �����ϱ� ���� GetJNetSession NULL ��ȯ�� ���� ���� �۾��� ���� �ʵ��� �ӽ������� ��ġ�Ѵ�.
							if (registDisconnection(hID)) {
								m_EventHandler->OnClientDisconnect(hID);
							}
						}

						break;
					}
					else if (recvLen == 0) {
						// ��������� ���� ���� ���� ��û (FIN)

						//if (eventHandler->OnClientDisconnect(hID)) {
						//	// OnClientDisconnect �̺�Ʈ�� �ް�, true�� ��ȯ�ϸ� �ھ� ������ ���� ���� ó��
						//	Disconnect(hID);
						//}

						// �� ���� �б�� ���������� �ߺ� ������ ���� ���� ������ ����
						if (registDisconnection(hID)) {
							m_EventHandler->OnClientDisconnect(hID);
						}
						break;
					}
					else {
						client->recvBuff.DirectMoveEnqueueOffset(recvLen);

						while (true) {
							stMSG_HDR hdr;
							client->recvBuff.Peek(&hdr);
							if (sizeof(hdr) + hdr.bySize > client->recvBuff.GetUseSize()) {
								break;
							}

							if (m_StupMap.find(hdr.byType) != m_StupMap.end()) {
								m_StupMap[hdr.byType]->ProcessReceivedMessage(hID, client->recvBuff);
							}
							else {
								DebugBreak();
							}
						}
					}
				}
			}
			session = session->nextSession;
		}
	}
}
void JNetCoreServer::sendSet() {
	stJNetSession* session = m_SessionManager.GetSessionFront();
	bool endFlag = false;
	for (int sidx = 0; !endFlag; sidx++) {
		if (m_RemoteWriteSets.size() <= sidx) {
			m_RemoteWriteSets.push_back(fd_set());
		}
		fd_set& remoteWriteSet = m_RemoteWriteSets[sidx];
		FD_ZERO(&remoteWriteSet);
		bool setFlag = false;

		for (int idx = 0; idx < FD_SETSIZE; idx++) {
			const stJNetSession* client = nullptr;
			if (session == nullptr) {
				endFlag = true;
				break;
			}
			else {
				client = session;
			}

			FD_SET(client->sock, &remoteWriteSet);
			setFlag = true;

			session = session->nextSession;
		}
		if (setFlag) {
			timeval tval = { 0, 0 };
			int retSel = select(0, nullptr, &remoteWriteSet, nullptr, &tval);
			if (retSel == SOCKET_ERROR) {
				ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::sendSet()", L"select(..) == SOCKET_ERROR", WSAGetLastError());
				DebugBreak();
			}
		}
	}
}
void JNetCoreServer::send() {
	stJNetSession* session = m_SessionManager.GetSessionFront();

	bool endFlag = false;
	for (int sidx = 0; !endFlag; sidx++) {
		for (int idx = 0; idx < FD_SETSIZE; idx++) {
			stJNetSession* client = nullptr;
			HostID hID = 0;
			if (session == nullptr) {
				endFlag = true;
				break;
			}
			else {
				client = session;
				hID = session->hostID;
			}

			fd_set& remoteWriteSet = m_RemoteWriteSets[sidx];
			if (FD_ISSET(client->sock, &remoteWriteSet)) {
				while (client->sendBuff.GetUseSize() > 0) {
					int len = client->sendBuff.GetUseSize();
					uint32 dirDeqSize = client->sendBuff.GetDirectDequeueSize();
					int sendLen;
					//if (len == dirDeqSize) {
					//	sendLen = ::send(client->sock, reinterpret_cast<const char*>(client->sendBuff->GetDequeueBufferPtr()), len, 0);
					//}
					//else if(len > dirDeqSize) {
					//	sendLen = ::send(client->sock, reinterpret_cast<const char*>(client->sendBuff->GetDequeueBufferPtr()), dirDeqSize, 0);
					//}
					//else {
					//	cout << "�ǵ����� ���� �帧: sendBuff->GetUseSize < sendBuff->dirDeqSize" << endl;
					//	assert(false);
					//}

					// �ǵ����� ���� �帧
					assert(len >= dirDeqSize);

					if (len > dirDeqSize) {
						len = dirDeqSize;
					}
					sendLen = ::send(client->sock, reinterpret_cast<const char*>(client->sendBuff.GetDequeueBufferPtr()), len, 0);

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
							if (registDisconnection(hID)) {
								m_EventHandler->OnClientDisconnect(hID);
							}
						}
						break;
					}
					else {
						bool skipFlag = false;

						if (sendLen < len) {
							//Q.select ��(+�� - ���ŷ ����)���� send �� ��û length���� ���� �����ٸ� ������ �� �����ΰ� ?
							//	(1) �۽��� �۽� ���� ����
							//	(2) ������ ���� ���� ����
							//
							//	���� �� ��Ȳ�� �߻��ϸ� ��� ��ó�� ���ΰ� ?
							//	(1) �׳� ���´�.
							//	(2) ���ݴ� ��ٸ���(?)

#if defined (PRINT_CONSOLE_LOG_ON)
							std::cout << "[SEND] sendLen < len : TCP �۽� ���� ���� ����" << std::endl;
							// �Ǵ� ����� ���� ���� ���� ����??
							std::cout << "[SEND] sendLen < len : ����� TCP ���� ���� ���� ����??" << std::endl;
							ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::send", L"sendLen < len");
#endif

							// �ϴ� ������ ���� �ʰ� �۽� ������ ���� ������ �ѱ�
							//if (Disconnect(hID)) {
							//	eventHandler->OnClientDisconnect(hID);
							//}

							//break;
							skipFlag = true;
						}

						client->sendBuff.DirectMoveDequeueOffset(sendLen);
						if (skipFlag) {
							break;
						}
					}
				}
			}
			session = session->nextSession;
		}
	}
}

bool JNetCoreServer::registDisconnection(HostID hostID)
{
	if (deleteCount++ > DELETE_LIMIT) {
		return false;
	}

	auto iter = m_DisconnectWaitSet.find(hostID);
	if (iter != m_DisconnectWaitSet.end()) {
		return false;
	}

	m_DisconnectWaitSet.insert(hostID);
	return true;
}

void JNetCoreServer::cleanUpSession()
{
	deleteCount = 0;

	for (HostID host : m_DisconnectWaitSet) {
		m_SessionManager.DeleteSession(host);
	}
	m_DisconnectWaitSet.clear();
}
