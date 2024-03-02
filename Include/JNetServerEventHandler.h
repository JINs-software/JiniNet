#pragma once
#include "JNetEventHandler.h"
#include "JNetCoreDefine.h"


class JNetServerEventHandler : public JNetEventHandler
{
public:
	virtual void OnReadyToAccept() {}
	virtual bool OnConnectRequest() { return true; }
	virtual void OnClientJoin(HostID remote) {}
	virtual bool OnClientDisconnect(HostID remote) { return true; }
};

