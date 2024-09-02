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

	void Disconnect(HostID remoteID);	// ���� ���� ��û

protected:
	inline stJNetSession* GetJNetSession(HostID remoteID) {
		return netcore->GetJNetSession(remoteID);
	}
	void Send(HostID remoteID, JBuffer& msg);

};

