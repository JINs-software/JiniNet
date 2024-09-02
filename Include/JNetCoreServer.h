#pragma once
#include "JNetworkCore.h"
#include "JNetServerEventHandler.h"
#include "JNetProxy.h"
#include "JNetStub.h"

class JNetCoreServer : public JNetworkCore
{
private:
	SOCKET	m_ListenSocket;
	fd_set	m_AcceptSet;

	JNetSessionManager	m_SessionManager;

	std::set<HostID>	m_DisconnectWaitSet;
	std::set<HostID>	m_ForcedDisconnectedSet;

	std::vector<fd_set> m_RemoteReadSets;
	std::vector<fd_set> m_RemoteWriteSets;

	JNetServerEventHandler* m_EventHandler;

	std::unordered_map<RpcID, JNetStub*> m_StupMap;

	const uint16	DELETE_LIMIT = 100;
	uint16			deleteCount = 0;
	bool			isOneWay = false;

public:
	JNetCoreServer(bool oneway) 
		: m_EventHandler(nullptr), isOneWay(oneway)
	{}
	void AttachEventHandler(JNetServerEventHandler* eventHandler);
	void AttachProxy(JNetProxy* proxy, BYTE unique);
	void AttachStub(JNetStub* stub, BYTE unique);
	void AttachBatchProcess(JNetBatchProcess* batch);

	bool Init(uint16 port, std::string IP = "") override;

	stJNetSession* GetJNetSession(HostID hostID) override;
	void RequestDisconnection(HostID hostID) override;

private:
	void Receive() override;
	void Send() override;

private:
	void receiveSet();
	void receive();
	void sendSet();
	void send();

private:
	bool registDisconnection(HostID hostID);
	void cleanUpSession();
};

