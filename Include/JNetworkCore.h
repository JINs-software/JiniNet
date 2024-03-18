#pragma once
#include <cassert>
#include "SocketUtil.h"
#include "JBuffer.h"
#include "JNetCoreConfig.h"
#include "JNetSession.h"
#include "JNetPool.h"

// 페이지 폴트 확인
#include <Psapi.h>
#pragma comment(lib, "psapi")
//#define CHECK_PAGE_FAULT

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

	unsigned int connectedSessionCount = 0;

	SessionManager() {
		sessionPool = new JiniPool(sizeof(stJNetSession), HOST_ID_LIMIT);
		remoteVec.resize(HOST_ID_LIMIT, nullptr);
		//for (uint32 i = HOST_ID_LIMIT - 1; i >= 3; i--) {
		for (uint32 i = 3; i < HOST_ID_LIMIT; i++) {
			availableID.push(i);	// 사용될 수 있는 ID는 3 ~ (HOST_ID_LIMIT - 1)
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
			assert(remoteVec[hostID] != NULL);
			

			// placement_new
			//new (remoteVec[hostID]) stJNetSession(sock, recvBuffSize, sendBuffSize);
			new (remoteVec[hostID]) stJNetSession(sock, hostID);

			if (frontSession == nullptr) {
				frontSession = remoteVec[hostID];
			}
			else {
				remoteVec[hostID]->nextSession = frontSession;
				assert(frontSession->prevSession == NULL);		// 로직 상 기존 frontSession의 prev는 항상 NULL이어야만 함.
				frontSession->prevSession = remoteVec[hostID];
				frontSession = remoteVec[hostID];
			}

			connectedSessionCount++;
		}
	}
	// Delete
	inline void DeleteSession(HostID hostID) {
		if (remoteVec[hostID] == NULL) {
			// 코어 측에서 삭제와 컨텐츠 쪽에서의 삭제 요청이 한 번의 게임 루프에서 동시에 발생할 수 있나?

			//assert(remoteVec[hostID] != NULL);	// 라이브러리 단에서 동일 ID에 대해 중복 삭제가 진행되는지 체크
			// 라이브러리 차원에서 연결 종료 또는 강제 종료를 감지하여 deleteSet에 ID를 담고 일괄적으로 삭제하는 작업,
			// 그리고 동일 루프에 캐릭터가 HP가 0이 되어 ForcedDisconnect 함수를 호출하여 라이브러리에 forcedDeleteSet에 ID를 담고 일괄적으로 삭제하는 작업
			// 이 두 작업이 한 번의 게임 루프에서 일어날 수 있다. 따라서 일단 임시방편으로 예외가 아닌 것으로 판단하고 assert를 지운다.
			return;
		}

		// 위에서 예외 처리한 것처럼 한번의 게임 루프에서 코어 측, 컨텐츠 요청 측 삭제가 하나의 ID에 중복될 수 있음을 고려
		// 기존에 한 번의 루프 내 중복 삭제, 중복 가용 ID 큐 삽입으로 인해 컨텐츠 단에서 충돌이 발생하였음(CreateFighter).
		// 이미 활성화된 ID가 클라이언트 맵에 존재하는데 같은 ID로 생성을 시도하려는 충돌 발생
		availableID.push(hostID);

		if (remoteVec[hostID] == frontSession) {
			//frontSession = nullptr;
			frontSession = frontSession->nextSession;
			if (frontSession != NULL) {
				frontSession->prevSession = NULL;
			}

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

		assert(connectedSessionCount != 0);
		connectedSessionCount--;
	}

	// GetSessionFront
	inline stJNetSession* GetSessionFront() {
		return frontSession;
	}

	inline unsigned int GetConnectedSessionCount() {
		return connectedSessionCount;
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

	////////////////////////////////////
	// 한 번에 삭제할 수 있는 세션 제한
	////////////////////////////////////
	const uint16 deleteLimit = 10;
	uint16 deleteCnt = 0;

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
		if (deleteCnt++ > deleteLimit) {
			return false;
		}

		if (disconnectedSet.find(remote) == disconnectedSet.end()) {
			//cout << "deleteCnt: " << deleteCnt << endl;
			//cout << "disconnectedSet.size(): " << disconnectedSet.size() << endl;
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

