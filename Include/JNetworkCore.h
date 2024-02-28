#pragma once
#include "SocketUtil.h"
#include "JBuffer.h"
#include "JNetMsgConfig.h"
#include "JNetSession.h"
#include "JNetPool.h"

// 페이지 폴트 확인
#include <Psapi.h>
#pragma comment(lib, "psapi")
//#define CHECK_PAGE_FAULT

/*** REMOTE 관리 ***/
//#define REMOTE_MAP
#define REMOTE_VEC

/*** 프록시의 다이렉트 세션 접근 ***/
#define DIRECT_ACCESS_TO_JNETSESSION

/*** 관리 메시지 번호 ***/
#define JNET_UNIQUE_MSG_NUM 0x88;
#define JNET_MSG_ALLOC 100

#define SERVER_HOST_ID 0

struct SessionManager {
	JiniPool* sessionPool;
	std::vector<stJNetSession*> remoteVec;
	// 코어 - 서버 로직 간 삭제 동기화 이슈 발생(ID 충돌)
	//std::stack<HostID> availableID;
	std::queue<HostID> availableID;
	stJNetSession* frontSession;
	//stJNetSession endSession;

	SessionManager() {
		sessionPool = new JiniPool(sizeof(stJNetSession), HOST_ID_LIMIT);
		remoteVec.resize(HOST_ID_LIMIT, nullptr);
		//for (uint32 i = HOST_ID_LIMIT - 1; i >= 3; i--) {
		for(uint32 i = 3; i < HOST_ID_LIMIT; i++) {
			availableID.push(i);
		}

		frontSession = nullptr;
		//endSession.prevSession = nullptr;
	}
	~SessionManager() {
		delete sessionPool;
	}

	// Get
	inline stJNetSession* GetSession(HostID hostID) {
		return remoteVec[hostID];
	}
	// Set
	inline bool SetSession(SOCKET sock, UINT recvBuffSize, UINT sendBuffSize, HostID& hostID) {
		if (availableID.empty()) {
			return false;
		}
		else {
			hostID = availableID.front(); //availableID.top();
			availableID.pop();
			remoteVec[hostID] = reinterpret_cast<stJNetSession*>(sessionPool->AllocMem());

			// placement_new
			new (remoteVec[hostID]) stJNetSession(sock, recvBuffSize, sendBuffSize);
			remoteVec[hostID]->hostID = hostID;
			
			if (frontSession == nullptr) {
				frontSession = remoteVec[hostID];
			}
			else {
				remoteVec[hostID]->nextSession = frontSession;
				frontSession->prevSession = remoteVec[hostID];
				frontSession = remoteVec[hostID];
			}
		}
	}
	// Delete
	inline void DeleteSession(HostID hostID) {
		availableID.push(hostID);
		if (remoteVec[hostID] == frontSession) {
			//frontSession = nullptr;
			frontSession = frontSession->nextSession;
		}
		else {
			remoteVec[hostID]->prevSession->nextSession = remoteVec[hostID]->nextSession;
			if (remoteVec[hostID]->nextSession != nullptr) {
				remoteVec[hostID]->nextSession->prevSession = remoteVec[hostID]->prevSession;
			}
		}

		// 명시적 소멸자 호출
		remoteVec[hostID]->~stJNetSession();
		sessionPool->ReturnMem(reinterpret_cast<BYTE*>(remoteVec[hostID]));
		remoteVec[hostID] = nullptr;
	}

	// GetSessionFront
	inline stJNetSession* GetSessionFront() {
		return frontSession;
	}
};

class JNetworkCore
{
	friend class JNetProxy;
private:
	WSADATA wsaData;
protected:
#ifdef REMOTE_MAP
	std::map<HostID, stJNetSession*> remoteMap;
#endif
#ifdef REMOTE_VEC
	SessionManager sessionMgr;
#endif // REMOTE_VEC
	std::set<HostID> disconnectedSet;
	std::set<HostID> forcedDisconnectedSet;
	SOCKET sock;
protected:
	fd_set readSet;
	fd_set writeSet;

public:
	JNetworkCore();
	inline bool Receive() {
		if (!receiveSet()) {
			return false;
		}
		//return receive();
		bool ret = receive();
		batchDisconnection();
		return ret;
	}
	inline bool Send() {
		if (!sendSet()) {
			return false;
		}
#ifdef CHECK_PAGE_FAULT
		HANDLE procHandle = GetCurrentProcess();
		PROCESS_MEMORY_COUNTERS pmcBefore;
		PROCESS_MEMORY_COUNTERS pmcAfter;
		if (!GetProcessMemoryInfo(procHandle, &pmcBefore, sizeof(pmcBefore))) {
			std::cout << "GetProcessMemoryInfo ERROR" << std::endl;
			return false;
		}
#endif // CHECK_PAGE_FAULT
		bool ret = send();
#ifdef CHECK_PAGE_FAULT
		if (!GetProcessMemoryInfo(procHandle, &pmcAfter, sizeof(pmcAfter))) {
			std::cout << "GetProcessMemoryInfo ERROR" << std::endl;
			return false;
		}
		std::cout << pmcBefore.PageFaultCount << std::endl;
		std::cout << pmcAfter.PageFaultCount << std::endl;
		std::cout << "page fault: " << pmcAfter.PageFaultCount - pmcBefore.PageFaultCount << std::endl;
#endif // CHECK_PAGE_FAULT
		batchDisconnection();
		return ret;
	}
	inline bool Disconnect(HostID remote) {
		if (disconnectedSet.find(remote) == disconnectedSet.end()) {
			disconnectedSet.insert(remote);
			return true;
		}
		else {
			return false;
		}
	}
	inline bool TwoWayDisconnect(HostID remote) {
		if (forcedDisconnectedSet.find(remote) == forcedDisconnectedSet.end()) {
			forcedDisconnectedSet.insert(remote);
			return true;
		}
		else {
			return false;
		}
	}
	inline bool ForcedDisconnect(HostID remote) {
		if (forcedDisconnectedSet.find(remote) == forcedDisconnectedSet.end()) {
			forcedDisconnectedSet.insert(remote);
			return true;
		}
		else {
			return false;
		}
	}

protected:
	virtual bool receiveSet();
	virtual bool receive() = 0;
	virtual bool sendSet();
	virtual bool send() = 0;

	void batchDisconnection();

	void ERROR_EXCEPTION_WINDOW(const WCHAR* wlocation, const WCHAR* wcomment, int errcode = -999999);

};

