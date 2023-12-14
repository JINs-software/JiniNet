#include "JNetworkCore.h"

bool JNetworkCore::receiveSet()
{
	FD_ZERO(&readSet);
	FD_SET(sock, &readSet);
	timeval tval = { 0, 0 };
	int retSel = select(0, &readSet, nullptr, nullptr, nullptr);
	if (retSel == SOCKET_ERROR) {
		ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::ReceiveSet()", L"select(..) == SOCKET_ERROR", WSAGetLastError());
		return false;
	}

	return true;
}

//bool JNetworkCore::sendSet()
//{
//	//FD_ZERO(&writeSet);
//	//FD_SET(sock, &writeSet);
//	//timeval tval = { 0, 0 };
//	//int retSel = select(0, nullptr, &writeSet, nullptr, nullptr);
//	//if (retSel == SOCKET_ERROR) {
//	//	ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::ReceiveSet()", L"select(..) == SOCKET_ERROR", WSAGetLastError());
//	//	return false;
//	//}
//	//
//	////if (FD_ISSET(sock, &writeSet)) {
//	//Send();
//	////}
//
//	//FD_ZERO(&writeSet);
//	//bool isEmptySet = true;
//	//for (const SOCKET& s : remoteSet) {
//	//	FD_SET(s, &writeSet);
//	//	isEmptySet = false;
//	//}
//	//timeval tval = { 0, 0 };
//	//if (!isEmptySet) {
//	//	int retSel = select(0, nullptr, &writeSet, nullptr, nullptr);
//	//	if (retSel == SOCKET_ERROR) {
//	//		ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::SendSet()", L"select(..) == SOCKET_ERROR", WSAGetLastError());
//	//		return false;
//	//	}
//	//}
//	
//	//return true;
//}

//bool JNetworkCore::SendMsgs(SOCKET remote, JBuffer& jbuff)
//{
//	if (remoteSet.find(remote) == remoteSet.end()) {
//		return false;
//	}
//
//	if (FD_ISSET(remote, &writeSet)) {
//		if (jbuff.GetUseSize() > 0) {
//			int32 sendLen = send(remote, reinterpret_cast<const char*>(jbuff.GetDequeueBufferPtr()), jbuff.GetDirectDequeueSize(), 0);
//			if (sendLen == SOCKET_ERROR) {
//				int32 errcode = WSAGetLastError();
//				if (errcode == WSAEWOULDBLOCK) {
//					ERROR_EXCEPTION_WINDOW(L"[Server Loop]: 클라이언트 소켓 송신", L"sendLen == SOCKET_ERROR", errcode);
//				}
//				else {
//					//deleteClientSet.insert(client->snickyInfo.ID);
//					//for (auto other = ClientMap.begin(); other != ClientMap.end(); other++) {
//					//	ClientSession* otherClient = other->second;
//					//	if (otherClient->snickyInfo.ID != client->snickyInfo.ID) {
//					//		DELETE_SC_MSG_Enqueue(otherClient->sendBuff, client->snickyInfo.ID);
//					//	}
//					//}
//				}
//			}
//			else {
//				jbuff.DirectMoveDequeueOffset(sendLen);
//			}
//		}
//	}
//}
