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

	bool Disconnect(HostID remoteID);
	bool ForcedDisconnect(HostID remoteID);	// ���� ���� ��û

protected:
	inline stJNetSession* GetJNetSession(HostID remoteID) {
		return netcore->sessionMgr.remoteVec[remoteID];
	}
	void Send(HostID remoteID, JBuffer& msg);

};

