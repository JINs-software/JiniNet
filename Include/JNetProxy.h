#pragma once
#include "JBuffer.h"
#include "JNetworkCore.h"

class JNetProxy
{
public:
	JNetworkCore* netcore;
	BYTE uniqueNum;

public:
	virtual RpcID* GetRpcList() { return nullptr; }
	virtual int GetRpcListCount() { return 0; }

protected:
	void Send(HostID remoteID, JBuffer& msg) {
		if (netcore->remoteMap.find(remoteID) != netcore->remoteMap.end()) {
			stJNetSession* remote = netcore->remoteMap[remoteID];
			if(remote->sendBuff->GetFreeSize() >= msg.GetUseSize()) {
				remote->sendBuff->Enqueue(msg.GetDequeueBufferPtr(), msg.GetUseSize());
			}
		}
	}
};

