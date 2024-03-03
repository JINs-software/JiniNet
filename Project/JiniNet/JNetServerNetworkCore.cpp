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
	//// 논-블로킹 소켓 전환
	u_long on = 1;
	if (ioctlsocket(sock, FIONBIO, &on) == SOCKET_ERROR) {
		ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::Start ", L"ioctlsocket(..) == SOCKET_ERROR");
	}
	// SO_REUSEADDR 옵션 적용
	int32 optval = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval)) == SOCKET_ERROR) {
		ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::Start ", L"setsocket(..SO_REUSEADDR) == SOCKET_ERROR");
	}
	// 서버 측에서 강제 종료 요청을 통한 종료이든, 클라이언트 측의 정상/비정상 종료 처리 시작이든 
	// TCP 강제 종료를 수행
	// => 링커 1/0 옵션 설정 (TCP 대기 소켓에 적용하여 accept 함수로 리턴되는 소켓은 자동으로 이 옵션으로 설정됨
	LINGER lingerOptval;
	lingerOptval.l_onoff = 1;
	lingerOptval.l_linger = 0;
	if (setsockopt(sock, SOL_SOCKET, SO_LINGER, (char*)&lingerOptval, sizeof(lingerOptval)) == SOCKET_ERROR) {
		ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::Start ", L"setsocket(..SO_LINGER) == SOCKET_ERROR");
	}

	//int option = TRUE;               //네이글 알고리즘 on/off
	//setsockopt(sock,             //해당 소켓
	//	IPPROTO_TCP,          //소켓의 레벨
	//	TCP_NODELAY,          //설정 옵션
	//	(const char*)&option, // 옵션 포인터
	//	sizeof(option));      //옵션 크기

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
	// 리슨 소켓 등록은 receive 함수에서 클라이언트 select 루프에서 진행한다.
	// 더 빈번하게 연결 요청을 확인하기 위해서이다.
	//JNetworkCore::receiveSet();

	/* 클라이언트 통신 소켓 64 초과 로직 추가 */
//#ifdef REMOTE_MAP
#if defined(REMOTE_MAP)
	auto iter = remoteMap.begin();
#elif defined(REMOTE_VEC)
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
	// TO DO: 클라이언트 통신 소켓 저장 및 아이디 공유
	// -> JNetServer에서 판단 및 진행

#ifdef REMOTE_MAP
	auto iter = remoteMap.begin();
#endif // REMOTE_MAP
#ifdef REMOTE_VEC
	stJNetSession* session = sessionMgr.GetSessionFront();
#endif // REMOTE_VEC
	bool endFlag = false;
	for (int sidx = 0; !endFlag; sidx++) {
		/////////////////////////////////////////////////
		// Listen Socket 확인 
		// 리슨 소켓 확인 작업을 더 빈번하게 하도록 함
		/////////////////////////////////////////////////
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

		//////////////////////////////////////////////////////////////////
		// 64개 이하의 클라이언트 세션 소켓 확인 
		//////////////////////////////////////////////////////////////////
		for (int idx = 0; idx < FD_SETSIZE; idx++) {
			stJNetSession* client = nullptr;
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

							// (1) 세션 삭제는 1차적으로 이벤트 핸들러를 거쳐 컨텐츠 단에서 삭제하도록 요청
							//     콘텐츠의 DeleteFighter 함수 호출
							// (2) 추후 일괄적으로 네트워크 코어 측에서 관리하는 삭제 세션 셋 일괄 삭제
							if (Disconnect(hID)) {
#if defined(PRINT_CONSOLE_LOG_ON)
								cout << "recv() return SOCKET_ERROR(" << errCode << ") | hostID: " << hID << endl;
#endif
								eventHandler->OnClientDisconnect(hID);
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
						if (Disconnect(hID)) {
#if defined(PRINT_CONSOLE_LOG_ON)
							cout << "recv() return 0, TCP 정상 종료 | hostID: " << hID << endl;
#endif
							eventHandler->OnClientDisconnect(hID);
						}
						break;
					}
					else {
						client->recvBuff.DirectMoveEnqueueOffset(recvLen);

						if (!oneway) {
							stJMSG_HDR hdr;
							client->recvBuff.Peek(&hdr);
							if (stupMap.find(hdr.msgID) != stupMap.end()) {
								stupMap[hdr.msgID]->ProcessReceivedMessage(hID, client->recvBuff);
							}
							else {
								ERROR_EXCEPTION_WINDOW(L"receive()", L"Undefined Message");
							}
						}
						else {
							stupMap[ONEWAY_RPCID]->ProcessReceivedMessage(hID, client->recvBuff);
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
			stJNetSession* client = nullptr;
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
							if (Disconnect(hID)) {
#if defined(PRINT_CONSOLE_LOG_ON)
								cout << "send() return SOCKET_ERROR( " << errCode << ") | hostID: " << hID << endl;
#endif
								eventHandler->OnClientDisconnect(hID);
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