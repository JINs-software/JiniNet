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
	// 리슨 소켓 및 Accept fd set 초기화
	m_ListenSocket = CreateWindowSocket_IPv4(true);
	FD_ZERO(&m_AcceptSet);

	// 논-블로킹 소켓 전환
	u_long on = 1;
	if (ioctlsocket(m_ListenSocket, FIONBIO, &on) == SOCKET_ERROR) {
		ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::Start ", L"ioctlsocket(..) == SOCKET_ERROR");
	}

	// SO_REUSEADDR 옵션 적용
	int32 optval = 1;
	if (setsockopt(m_ListenSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval)) == SOCKET_ERROR) {
		ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::Start ", L"setsocket(..SO_REUSEADDR) == SOCKET_ERROR");
	}

	// 서버 측에서 강제 종료 요청을 통한 종료이든, 클라이언트 측의 정상/비정상 종료 처리 시작이든 
	// TCP 강제 종료를 수행
	// => 링커 1/0 옵션 설정 (TCP 대기 소켓에 적용하여 accept 함수로 리턴되는 소켓은 자동으로 이 옵션으로 설정됨
	LINGER lingerOptval;
	lingerOptval.l_onoff = 1;
	lingerOptval.l_linger = 0;
	if (setsockopt(m_ListenSocket, SOL_SOCKET, SO_LINGER, (char*)&lingerOptval, sizeof(lingerOptval)) == SOCKET_ERROR) {
		ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::Start ", L"setsocket(..SO_LINGER) == SOCKET_ERROR");
	}

	// IP 지정 방식
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
	// 리슨 소켓 등록은 receive 함수에서 클라이언트 select 루프에서 진행한다.
	// 더 빈번하게 연결 요청을 확인하기 위해서이다.
	//JNetworkCore::receiveSet();

	/* 클라이언트 통신 소켓 64 초과 로직 추가 */
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
		// Listen Socket 확인 (리슨 소켓 확인 작업을 더 빈번하게 하도록 함)
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
		// 64개 이하의 클라이언트 세션 소켓 확인 
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
			if (FD_ISSET(client->sock, &remoteReadSet)) {	// client->sock은 논-블로킹 모드의 소켓
				while (true) {
					// 다이렉트로 받을 수 있는 만큼 씩 받는다. 만약 TCP 수신 버퍼에 데이터가 남아있는데, 다이렉트 용량이 부족하다면 while(true)루프로 받을 수 있다.
					// 그러나 'JBUFF_DIRPTR_MANUAL_RESET'를 정의하지 않으면 JBuffer는 인큐, 디큐 포인터가 같아질 때 다시 각 포인터를 다시 메모리 버퍼 시작점으로 이동시킨다.
					// 이에 다이렉트 용량이 부족해지는 경우가 드물것으로 예상된다.
					int recvLen = recv(client->sock, reinterpret_cast<char*>(client->recvBuff.GetEnqueueBufferPtr()), client->recvBuff.GetDirectEnqueueSize(), 0);

					if (recvLen == SOCKET_ERROR) {
						// TO DO: 에러 처리
						// 본래 select 모델이기에 WSAEWOULDBLOCK 에러가 발생하지 않음을 기대한다. 
						// 하지만 FD_ISSET에서 TRUE가 반환되고, 수신 작업을 할 때, 다이렉트 디큐 사이즈로 받기에 받을 수 있는 데이터보다 더 적게 받을 수 있는 가능성이 있다. 
						// 따라서 while 루프를 돌아 추가적인 수신을 시도한다. 이러한 경우 WSAWOULDBLOCK 에러가 발생할 수 있음을 고려해야 한다. 

						int errCode = WSAGetLastError();
						if (errCode != WSAEWOULDBLOCK) {
							//std::cout << "[JNet::RECV] HostID: " << hID << "WSAGetLastError: " << errCode << std::endl;
							//if (eventHandler->OnClientDisconnect(hID)) {
							//	// OnClientDisconnect 이벤트를 받고, true를 받환하면 코어 측에서 연결 종료 처리
							//	Disconnect(hID);
							//}
							//=> 이미 삭제 요청된 클라이언트에 중복으로 에러가 뜰 수 있음

							// 세션 삭제는 1차적으로 이벤트 핸들러를 거쳐 컨텐츠 단에서 삭제하도록 요청하도록 하였다.
							//		(컨텐츠)
							//		-> 콘텐츠의 DeleteFighter 함수 호출, g_DeleteClientSet<hostID, netCoreSideFlag>.insert(..);
							//		-> g_DeleteClientSet에 삽입된 삭제 대상 ID들에 대해 BatchProcess에서 일괄적으로 삭제함
							//			(FrameMove)
							//			- Receive
							//				- 삭제 대상 세션 일괄 삭제
							//			- BatchProcess	// BatchDelete
							//			- Send
							//				- 삭제 대상 세션 일괄 삭제
							// 그러나 분석해보니 위와 같이 콘텐츠 단의 DeleteFigther는 사실상 삭제 대상 플레이어 셋에 추가하는 것이고,
							// BatchProcess 단계에서 삭제 플레이어 셋에 대해 일괄적으로 삭제한다. 
							// 결국 라이브러리 세션 측 삭제가 먼저 일어난다는 것이다. 
							// 
							// BatchDelete에서는 삭제 대상 주변의 플레이어들에게 DEL_CHARACTER 메시지를 일괄적으로 전송하게 되는데, 
							// 주변 플레이어들의 링버퍼를 직접 참조하여 직렬적으로 메시지를 복사시킨다. 
							// 문제는 주변 플레이어를 참조하는데 있어 GetJNetSession 함수를 호출하는데, 이미 삭제한 세션이 있다면 NULL을 반환하게 된다.
							// 컨텐츠 플레이어 삭제 -> 세션 삭제 가정된 코드였기에 NULL 반환이 의심스러웠는데, 사실상 어쩔 수 없이 NULL이 반환되는 상황이다.
							// 전적으로 컨텐츠 쪽 삭제가 먼저 일어나도록 코드를 수정하기 보단 GetJNetSession NULL 반환에 대한 예외 작업을 하지 않도록 임시적으로 조치한다.
							if (registDisconnection(hID)) {
								m_EventHandler->OnClientDisconnect(hID);
							}
						}

						break;
					}
					else if (recvLen == 0) {
						// 상대측에서 정상 연결 종료 요청 (FIN)

						//if (eventHandler->OnClientDisconnect(hID)) {
						//	// OnClientDisconnect 이벤트를 받고, true를 받환하면 코어 측에서 연결 종료 처리
						//	Disconnect(hID);
						//}

						// 위 에러 분기와 마찬가지로 중복 삭제를 막기 위해 순서를 변경
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
					//	cout << "의도되지 않은 흐름: sendBuff->GetUseSize < sendBuff->dirDeqSize" << endl;
					//	assert(false);
					//}

					// 의도되지 않은 흐름
					assert(len >= dirDeqSize);

					if (len > dirDeqSize) {
						len = dirDeqSize;
					}
					sendLen = ::send(client->sock, reinterpret_cast<const char*>(client->sendBuff.GetDequeueBufferPtr()), len, 0);

					if (sendLen == SOCKET_ERROR) {
						// 에러 처리
						// TO DO: 에러 처리
						// select 모델이기에 WSAEWOULDBLOCK 에러가 발생하지 않음을 기대한다. 
						// 방어코드 
						int errCode = WSAGetLastError();
						if (errCode != WSAEWOULDBLOCK) {
							//std::cout << "[JNet::SEND] HostID: " << hID << "WSAGetLastError: " << errCode << std::endl;
							//if (eventHandler->OnClientDisconnect(hID)) {
							//	// OnClientDisconnect 이벤트를 받고, true를 받환하면 코어 측에서 연결 종료 처리
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
							//Q.select 모델(+논 - 블로킹 소켓)에서 send 시 요청 length보다 적게 보낸다면 문제는 두 가지인가 ?
							//	(1) 송신측 송신 버퍼 부족
							//	(2) 수신측 수신 버퍼 부족
							//
							//	만약 이 상황이 발생하면 어떻게 대처할 것인가 ?
							//	(1) 그냥 끊는다.
							//	(2) 조금더 기다린다(?)

#if defined (PRINT_CONSOLE_LOG_ON)
							std::cout << "[SEND] sendLen < len : TCP 송신 버퍼 공간 부족" << std::endl;
							// 또는 상대측 수신 버퍼 공간 부족??
							std::cout << "[SEND] sendLen < len : 상대측 TCP 수신 버퍼 공간 부족??" << std::endl;
							ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::send", L"sendLen < len");
#endif

							// 일단 연결을 끊지 않고 송신 루프의 다음 턴으로 넘김
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
