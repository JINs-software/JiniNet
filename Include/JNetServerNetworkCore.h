#pragma once
#include "JNetworkCore.h"
#include "JNetServerEventHandler.h"
#include "JNetMsgConfig.h"
#include "JNetStub.h"

#define SESSION_RECV_BUFF 1000
#define SESSION_SEND_BUFF 1000

struct stServerStartParam {
	std::string IP;
	uint16_t	Port;
};

class JNetServerNetworkCore : public JNetworkCore
{
private:
	JNetServerEventHandler* eventHandler;
	std::unordered_map<RpcID, JNetStub*> stupMap;
	fd_set remoteReadSet;

public:
	JNetServerNetworkCore();
	inline void AttachEventHandler(JNetServerEventHandler* eventHandler) {
		this->eventHandler = eventHandler;
	}
	inline void AttachStub(JNetStub* stub) {
		RpcID* rpcList = stub->GetRpcList();
		int rpcListCnt = stub->GetRpcListCount();
		for (int i = 0; i < rpcListCnt; i++) {
			stupMap.insert({ rpcList[i], stub });
		}
	}

	bool Start(const stServerStartParam param);

private:
	bool receiveSet() override;
	bool receive() override;
	bool sendSet() override;
	bool send();
};

