#pragma once
#include "JBuffer.h"
#include <WinSock2.h>
#include "JNetCoreDefine.h"
#include "JNetSessionConfig.h"
#include "JNetPool.h"

typedef unsigned int HostID;

struct stJNetSession {
	HostID hostID;
	SOCKET sock;
	//JBuffer* recvBuff;
	//JBuffer* sendBuff;
	// => JNetPool에서 세션 공간을 할당받는데, 내부에서 링퍼버를 new를 통한 동적 할당을 받는다면 풀을 사용하는 목적이 무의미하다.
	BYTE recvBuff_internal[SESSION_RECV_BUFF];
	BYTE sendBuff_internal[SESSION_SEND_BUFF];
	JBuffer recvBuff;
	JBuffer sendBuff;

	//#ifdef REMOTE_VEC
	stJNetSession* prevSession = nullptr;
	stJNetSession* nextSession = nullptr;
	//#endif // REMOTE_VEC

	stJNetSession(SOCKET _sock, HostID _hostID)
		: sock(_sock), hostID(_hostID), recvBuff(SESSION_RECV_BUFF, recvBuff_internal), sendBuff(SESSION_SEND_BUFF, sendBuff_internal)
	{}
	~stJNetSession() {
		closesocket(sock);
	}
};

class JNetSessionManager {
	JiniPool* m_SessionPool;
	std::vector<stJNetSession*> remoteVec;
	std::queue<HostID> availableID;		// vs stack<HostID>
										// 장점: ID 충돌 회피
										// 단점: 캐시 활용성에 있어 stack에 비해 저하
	stJNetSession* frontSession;

	unsigned int connectedSessionCount = 0;

public:
	JNetSessionManager() {
		m_SessionPool = new JiniPool(sizeof(stJNetSession), HOST_ID_LIMIT);
		remoteVec.resize(HOST_ID_LIMIT, nullptr);
		for (uint32 i = 3; i < HOST_ID_LIMIT; i++) {
			availableID.push(i);	// 사용될 수 있는 ID는 3 ~ (HOST_ID_LIMIT - 1)
		}

		frontSession = nullptr;
	}
	~JNetSessionManager() {
		delete m_SessionPool;
	}

	// Check
	inline bool SessionAllocable() {
		return !availableID.empty();
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
		
		hostID = availableID.front(); //availableID.top();
		availableID.pop();
		remoteVec[hostID] = reinterpret_cast<stJNetSession*>(m_SessionPool->AllocMem());
		if (remoteVec[hostID] == NULL) {
			DebugBreak();
			return false;
		}

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
		
		return true;
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
		m_SessionPool->ReturnMem(reinterpret_cast<BYTE*>(remoteVec[hostID]));
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