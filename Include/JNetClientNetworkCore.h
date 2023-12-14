#pragma once
#include <map>
#include <unordered_map>
#include "JNetworkCore.h"
#include "JNetClientEventHandler.h"
#include "JNetStub.h"

#define CLIENT_RECV_BUFF 1000
#define CLIENT_SEND_BUFF 1000

class JNetClientNetworkCore : public JNetworkCore
{
private:
	JNetClientEventHandler* eventHandler;
	std::unordered_map<RpcID, JNetStub*> stupMap;
	stJNetSession* session;

public:
	JNetClientNetworkCore();
	inline void AttachEventHandler(JNetClientEventHandler* eventHandler) {
		this->eventHandler = eventHandler;
	}
	inline void AttachStub(JNetStub* stub) {
		RpcID* rpcList = stub->GetRpcList();
		int rpcListCnt = stub->GetRpcListCount();
		for (int i = 0; i < rpcListCnt; i++) {
			stupMap.insert({ rpcList[i], stub });
		}
	}
	bool Connect();

private:
	bool receive() override;
	bool sendSet() override;
	bool send() override;
};

