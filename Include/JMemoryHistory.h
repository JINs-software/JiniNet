#pragma once

/**********************************/
/*
* [�޸� �Ҵ�, ���� ������ �������ִ� Ŭ����]
* 
* ���� ���̺� ������Ʈ�� ����ϱ� ���ؼ� �� ������ �߰� ������ �ʿ�
* => 1. �޸� �Ҵ� ���� ������ �迭���� ����Ʈ �������
* => 2. Ŭ������ ���� �迭 ���� �� 4, 8 ����Ʈ�� �� Ȯ���ϹǷ� ���ο��� ������ �Ҵ� �����Ϳ�
*		����ڿ��� ��ȯ�� �����ʹ� �ٸ� �� ����. (�̿� ���� ó���� ����)
*/

class JMemoryHistory
{
public:
	enum {
		LOF_FILE_NAME_LEN = 64,
		FILE_NAME_LEN = 128,
		ALLOC_INFO_MAX = 1000
	};

	// �޸� �Ҵ� ����
	struct stALLOC_INFO {
		void* pAlloc;				// �Ҵ���� ������
		int iSize;					// �Ҵ� ������
		char szFile[FILE_NAME_LEN];	// �Ҵ� ���� ��ġ
		int iLine;					// ���� ����
		bool bArray;				// �迭 ���� ����
	};

private:
	// �޸� �Ҵ� ������ ���� �迭
	stALLOC_INFO _allocInfos[ALLOC_INFO_MAX];
	char _logFileName[LOF_FILE_NAME_LEN];		// �α� ���� �̸��� ������ �迭

public:
	JMemoryHistory(const char* szLogFile = "AllocInfo");
	~JMemoryHistory();

private:
	bool SaveLogFile(void);
	bool NewAlloc(void* pPtr, char* szFile, int iLine, int iSize, bool bArray = false);
	bool Delete(void* pPtr, bool bArray = false);

	// new ������ �����ε� �Լ������� �� Ŭ������ ������ �� �ִ�.
	friend void* operator new(size_t size, char* File, int Line);
	friend void* operator new[](size_t size, char* File, int Line);
	friend void operator delete(void* p);
	friend void operator delete[](void* p);
};

