#include "JNetProxy.h"

RpcID* JNetProxy::GetRpcList() { return nullptr; }
int JNetProxy::GetRpcListCount() { return 0; }

void JNetProxy::Send(HostID remoteID, JBuffer& msg) {
	if (netcore->remoteMap.find(remoteID) != netcore->remoteMap.end()) {
		stJNetSession* remote = netcore->remoteMap[remoteID];
		if (remote->sendBuff->GetFreeSize() >= msg.GetUseSize()) {
			remote->sendBuff->Enqueue(msg.GetDequeueBufferPtr(), msg.GetUseSize());
		}
	}
}