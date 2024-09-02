#pragma once
#include "JBuffer.h"
#include "JNetworkCore.h"
#include "JNetRPC.h"

class JNetProxy
{
public:
	JNetworkCore* netcore;
	BYTE uniqueNum;

public:
	virtual RpcID* GetRpcList();
	virtual int GetRpcListCount();

	void Disconnect(HostID remoteID);	// 연결 종료 요청

protected:
	inline stJNetSession* GetJNetSession(HostID remoteID) {
		return netcore->GetJNetSession(remoteID);
	}
	void Send(HostID remoteID, JBuffer& msg);

};

