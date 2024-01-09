#pragma once
#include "JNetworkCore.h"
#include "JNetServerEventHandler.h"
#include "JNetMsgConfig.h"
#include "JNetStub.h"

#define SESSION_RECV_BUFF 10000
#define SESSION_SEND_BUFF 10000

struct stServerStartParam {
	std::string IP;
	uint16_t	Port;
};

class JNetServerNetworkCore : public JNetworkCore
{
private:
	JNetServerEventHandler* eventHandler;
	std::unordered_map<RpcID, JNetStub*> stupMap;
	//fd_set remoteReadSet;
	std::vector<fd_set> remoteReadSets;
	std::vector<fd_set> remoteWriteSets;
	bool oneway = false;

public:
	JNetServerNetworkCore();
	inline void SetOneway() { oneway = true; }
	inline void AttachEventHandler(JNetServerEventHandler* eventHandler) {
		this->eventHandler = eventHandler;
	}
	inline void AttachStub(JNetStub* stub) {
		if (!oneway) {
			RpcID* rpcList = stub->GetRpcList();
			int rpcListCnt = stub->GetRpcListCount();
			for (int i = 0; i < rpcListCnt; i++) {
				stupMap.insert({ rpcList[i], stub });
			}
		}
		else {
			if (!stupMap.empty()) {
				ERROR_EXCEPTION_WINDOW(L"JNetServerNetworkCore::AttachStub", L"Oneway 방식에서는 하나의 스텁만이 존재할 수 있음");
			}
			else {
				stupMap.insert({ ONEWAY_RPCID, stub });
			}
		}
	}

	bool Start(const stServerStartParam param);
	bool CloseConnection(HostID clientID);

private:
	bool receiveSet() override;
	bool receive() override;
	bool sendSet() override;
	bool send();
};

