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
	// => JNetPool���� ���� ������ �Ҵ�޴µ�, ���ο��� ���۹��� new�� ���� ���� �Ҵ��� �޴´ٸ� Ǯ�� ����ϴ� ������ ���ǹ��ϴ�.
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
										// ����: ID �浹 ȸ��
										// ����: ĳ�� Ȱ�뼺�� �־� stack�� ���� ����
	stJNetSession* frontSession;

	unsigned int connectedSessionCount = 0;

public:
	JNetSessionManager() {
		m_SessionPool = new JiniPool(sizeof(stJNetSession), HOST_ID_LIMIT);
		remoteVec.resize(HOST_ID_LIMIT, nullptr);
		for (uint32 i = 3; i < HOST_ID_LIMIT; i++) {
			availableID.push(i);	// ���� �� �ִ� ID�� 3 ~ (HOST_ID_LIMIT - 1)
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
			assert(frontSession->prevSession == NULL);		// ���� �� ���� frontSession�� prev�� �׻� NULL�̾�߸� ��.
			frontSession->prevSession = remoteVec[hostID];
			frontSession = remoteVec[hostID];
		}

		connectedSessionCount++;
		
		return true;
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