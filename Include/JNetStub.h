#pragma once
#include "JBuffer.h"
#include "JNetMsgConfig.h"
#include "JNetRPC.h"

class JNetStub
{
public:
	BYTE uniqueNum;

public:
	virtual RpcID* GetRpcList() { return nullptr; }
	virtual int GetRpcListCount() { return 0; }

	virtual bool ProcessReceivedMessage(HostID remote, JBuffer& jbuff);
};