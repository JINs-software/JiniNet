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
			cout << "netcore 송신 버퍼 크기 부족!" << endl;
			cout << "remote->sendBuff->GetFreeSize(): " << remote->sendBuff->GetFreeSize() << endl;
			cout << "remote->sendBuff->GetUseSize(): " << remote->sendBuff->GetUseSize() << endl;
			cout << "msg.GetUseSize(): " << msg.GetUseSize() << endl;
			assert(remote->sendBuff->GetFreeSize() >= msg.GetUseSize());
		}
}
#endif // REMOTE_MAP
#ifdef REMOTE_VEC
	if (netcore->remoteVec[remoteID] != nullptr) {
		stJNetSession* remote = netcore->remoteVec[remoteID];
		if (remote->sendBuff->GetFreeSize() >= msg.GetUseSize()) {
			remote->sendBuff->Enqueue(msg.GetDequeueBufferPtr(), msg.GetUseSize());
		}
		else {
			cout << "netcore 송신 버퍼 크기 부족!" << endl;
			cout << "remote->sendBuff->GetFreeSize(): " << remote->sendBuff->GetFreeSize() << endl;
			cout << "remote->sendBuff->GetUseSize(): " << remote->sendBuff->GetUseSize() << endl;
			cout << "msg.GetUseSize(): " << msg.GetUseSize() << endl;
			assert(remote->sendBuff->GetFreeSize() >= msg.GetUseSize());
		}
}
#endif // REMOTE_VEC
}

bool JNetProxy::Disconnect(HostID remoteID)
{
	return netcore->ForcedDisconnect(remoteID);
}
