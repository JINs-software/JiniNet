#pragma once
#include "JBuffer.h"
#include "JNetworkCore.h"

class JNetProxy
{
public:
	JNetworkCore* netcore;
	BYTE uniqueNum;

public:
	virtual RpcID* GetRpcList();
	virtual int GetRpcListCount();

	bool Disconnect(HostID remoteID);	// 연결 종료 요청

protected:
	void Send(HostID remoteID, JBuffer& msg);

};

