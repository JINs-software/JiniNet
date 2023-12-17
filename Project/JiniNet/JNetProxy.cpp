#include "JNetProxy.h"
#include <cassert>
#include <iostream>
using namespace std;

RpcID* JNetProxy::GetRpcList() { return nullptr; }
int JNetProxy::GetRpcListCount() { return 0; }

void JNetProxy::Send(HostID remoteID, JBuffer& msg) {
	if (netcore->remoteMap.find(remoteID) != netcore->remoteMap.end()) {
		stJNetSession* remote = netcore->remoteMap[remoteID];
		if (remote->sendBuff->GetFreeSize() >= msg.GetUseSize()) {
			remote->sendBuff->Enqueue(msg.GetDequeueBufferPtr(), msg.GetUseSize());
		}
		else {
			cout << "netcore 송신 버퍼 크기 부족!" << endl;
			assert(remote->sendBuff->GetFreeSize() >= msg.GetUseSize());
		}
	}
}

bool JNetProxy::Disconnect(HostID remoteID)
{
	return netcore->Disconnect(remoteID);
}
