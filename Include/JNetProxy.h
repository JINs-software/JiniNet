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

protected:
	void Send(HostID remoteID, JBuffer& msg);

};

