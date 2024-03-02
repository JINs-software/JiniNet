#pragma once
#include "JBuffer.h"
#include "JNetCoreDefine.h"
#include "JNetRPC.h"

class JNetStub
{
public:
	BYTE uniqueNum;

public:
	virtual RpcID* GetRpcList();
	virtual int GetRpcListCount();

	virtual bool ProcessReceivedMessage(HostID remote, JBuffer& jbuff);
};