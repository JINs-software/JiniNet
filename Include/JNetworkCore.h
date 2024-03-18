#pragma once
#include <cassert>
#include "SocketUtil.h"
#include "JBuffer.h"
#include "JNetCoreConfig.h"
#include "JNetSession.h"
#include "JNetPool.h"

// ������ ��Ʈ Ȯ��
#include <Psapi.h>
#pragma comment(lib, "psapi")
//#define CHECK_PAGE_FAULT

/*** ���� �޽��� ��ȣ ***/
#define JNET_UNIQUE_MSG_NUM 0x88;
#define JNET_MSG_ALLOC 100

#define SERVER_HOST_ID 0

struct SessionManager {
	JiniPool* sessionPool;
	std::vector<stJNetSession*> remoteVec;
	// �ھ� - ���� ���� �� ���� ����ȭ �̽� �߻�(ID �浹)
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
			availableID.push(i);	// ���� �� �ִ� ID�� 3 ~ (HOST_ID_LIMIT - 1)
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
				assert(frontSession->prevSession == NULL);		// ���� �� ���� frontSession�� prev�� �׻� NULL�̾�߸� ��.
				frontSession->prevSession = remoteVec[hostID];
				frontSession = remoteVec[hostID];
			}

			connectedSessionCount++;
		}
	}
	// Delete
	inline void DeleteSession(HostID hostID) {
		if (remoteVec[hostID] == NULL) {
			// �ھ� ������ ������ ������ �ʿ����� ���� ��û�� �� ���� ���� �������� ���ÿ� �߻��� �� �ֳ�?

			//assert(remoteVec[hostID] != NULL);	// ���̺귯�� �ܿ��� ���� ID�� ���� �ߺ� ������ ����Ǵ��� üũ
			// ���̺귯�� �������� ���� ���� �Ǵ� ���� ���Ḧ �����Ͽ� deleteSet�� ID�� ��� �ϰ������� �����ϴ� �۾�,
			// �׸��� ���� ������ ĳ���Ͱ� HP�� 0�� �Ǿ� ForcedDisconnect �Լ��� ȣ���Ͽ� ���̺귯���� forcedDeleteSet�� ID�� ��� �ϰ������� �����ϴ� �۾�
			// �� �� �۾��� �� ���� ���� �������� �Ͼ �� �ִ�. ���� �ϴ� �ӽù������� ���ܰ� �ƴ� ������ �Ǵ��ϰ� assert�� �����.
			return;
		}

		// ������ ���� ó���� ��ó�� �ѹ��� ���� �������� �ھ� ��, ������ ��û �� ������ �ϳ��� ID�� �ߺ��� �� ������ ���
		// ������ �� ���� ���� �� �ߺ� ����, �ߺ� ���� ID ť �������� ���� ������ �ܿ��� �浹�� �߻��Ͽ���(CreateFighter).
		// �̹� Ȱ��ȭ�� ID�� Ŭ���̾�Ʈ �ʿ� �����ϴµ� ���� ID�� ������ �õ��Ϸ��� �浹 �߻�
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

		// ����� �Ҹ��� ȣ��
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
	// �� ���� ������ �� �ִ� ���� ����
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

