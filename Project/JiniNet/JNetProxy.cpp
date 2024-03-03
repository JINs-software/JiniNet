#include "JNetProxy.h"
#include <cassert>
#include <iostream>
using namespace std;

RpcID* JNetProxy::GetRpcList() { return nullptr; }
int JNetProxy::GetRpcListCount() { return 0; }

void JNetProxy::Send(HostID remoteID, JBuffer& msg) {
#ifdef REMOTE_MAP
	if (netcore->remoteMap.find(remoteID) != netcore->remoteMap.end()) {
		stJNetSession* remote = netcore->remoteMap[remoteID];
		if (remote->sendBuff->GetFreeSize() >= msg.GetUseSize()) {
			remote->sendBuff->Enqueue(msg.GetDequeueBufferPtr(), msg.GetUseSize());
		}
		else {
			cout << "netcore �۽� ���� ũ�� ����!" << endl;
			cout << "remote->sendBuff->GetFreeSize(): " << remote->sendBuff->GetFreeSize() << endl;
			cout << "remote->sendBuff->GetUseSize(): " << remote->sendBuff->GetUseSize() << endl;
			cout << "msg.GetUseSize(): " << msg.GetUseSize() << endl;
			assert(remote->sendBuff->GetFreeSize() >= msg.GetUseSize());
		}
	}
#endif // REMOTE_MAP
#ifdef REMOTE_VEC && !DIRECT_ACCESS_TO_JNETSESSION
	stJNetSession* remote;
	if ((remote = netcore->sessionMgr.GetSession(remoteID)) != nullptr) {
		if (remote->sendBuff.GetFreeSize() >= msg.GetUseSize()) {
			remote->sendBuff.Enqueue(msg.GetDequeueBufferPtr(), msg.GetUseSize());
		}
		else {
			cout << "netcore �۽� ���� ũ�� ����!" << endl;
			cout << "remote->sendBuff->GetFreeSize(): " << remote->sendBuff.GetFreeSize() << endl;
			cout << "remote->sendBuff->GetUseSize(): " << remote->sendBuff.GetUseSize() << endl;
			cout << "msg.GetUseSize(): " << msg.GetUseSize() << endl;
			assert(remote->sendBuff.GetFreeSize() >= msg.GetUseSize());
		}
	}
#endif // REMOTE_VEC && !DIRECT_ACCESS_TO_JNETSESSION
}

//bool JNetProxy::Disconnect(HostID remoteID)
//{
//	return false;
//}

bool JNetProxy::ForcedDisconnect(HostID remoteID)
{
	return netcore->ForcedDisconnect(remoteID);
}
