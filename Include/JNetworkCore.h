#pragma once
#include <cassert>
#include "SocketUtil.h"
#include "JBuffer.h"
#include "JNetCoreConfig.h"
#include "JNetBatchProcess.h"
#include "JNetSession.h"

class JNetworkCore
{
	friend class JNetProxy;

private:
	WSADATA wsaData;

protected:
	JNetBatchProcess* m_BatchProcess;

public:
	JNetworkCore();
	~JNetworkCore();

	// called by game sever main
	virtual bool Init(uint16 port, std::string IP = "") = 0;
	virtual void Start(long msecPerFrame);

	// called by proxy
	virtual stJNetSession* GetJNetSession(HostID hostID) = 0;
	virtual void RequestDisconnection(HostID hostID) = 0;

private:
	virtual void Receive() = 0;
	virtual void Send() = 0;
	void FrameMove(uint16 frameDelta);

protected:
	void ERROR_EXCEPTION_WINDOW(const WCHAR* wlocation, const WCHAR* wcomment, int errcode = -999999);
};

