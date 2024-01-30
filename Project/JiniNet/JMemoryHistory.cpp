#include "JMemoryHistory.h"
#include <memory>
#include <ctime>

JMemoryHistory::JMemoryHistory(const char* szLogFile) {
	char szTime[17] = "";
	time_t timer;
	struct tm TM;

	memset(_allocInfos, 0, sizeof(stALLOC_INFO) * ALLOC_INFO_MAX);
	memset(_logFileName, 0, LOF_FILE_NAME_LEN);

	time(&timer);
	localtime_s(&TM, &timer);

	// YYYYMMDD_hhmmss �������� ����
	sprintf_s(szTime, 17, "%04d%02d%02d_%02d%02d%02d",
		TM.tm_year + 1900,
		TM.tm_mon + 1,
		TM.tm_mday,
		TM.tm_hour,
		TM.tm_min,
		TM.tm_sec);

	// �α� ���� �̸�: AllocInfo20240125_123045.txt
	strcat_s(_logFileName, LOF_FILE_NAME_LEN, szLogFile);
	strcat_s(_logFileName, LOF_FILE_NAME_LEN, szTime);
	strcat_s(_logFileName, LOF_FILE_NAME_LEN, ".txt");
}
JMemoryHistory::~JMemoryHistory() {
	SaveLogFile();
}

bool JMemoryHistory::SaveLogFile(void) {
	FILE* pLogFile;
	// �α� ���� ���� ���� open
	errno_t err = fopen_s(&pLogFile, _logFileName, "a");	// �ɼ� "a": ���ϸ� ���� ����� ���� �̾, ���ϸ� ���� ������ ����� ���� ����
	if (err != 0) {
		return false;
	}

	// �α� ���Ͽ� �ۼ� ���� �޸� ���� ���
	for (int iCnt = 0; iCnt < ALLOC_INFO_MAX; iCnt++) {
		if (_allocInfos[iCnt].pAlloc != NULL) {
			// ���� ����
			// ������ �޸� - [�޸� ������] [������] ���� ��ġ : �� ��ȣ
			// ������ �޸� - [�޸� ������] [������] ���� ��ġ : �� ��ȣ
			fprintf(pLogFile, "LEAK !! ");
			fprintf(pLogFile, "[0x%p] [0%7d] %s : %d \n",
				_allocInfos[iCnt].pAlloc,
				_allocInfos[iCnt].iSize,
				_allocInfos[iCnt].szFile,
				_allocInfos[iCnt].iLine);
		}
	}
	fclose(pLogFile);
	return true;
}
bool JMemoryHistory::NewAlloc(void* pPtr, char* szFile, int iLine, int iSize, bool bArray) {
	for (int iCnt = 0; iCnt < ALLOC_INFO_MAX; iCnt++) {
		_allocInfos[iCnt].bArray = bArray;
		_allocInfos[iCnt].pAlloc = pPtr;
		_allocInfos[iCnt].iLine = iLine;
		_allocInfos[iCnt].iSize = iSize;
		strcpy_s(_allocInfos[iCnt].szFile, FILE_NAME_LEN, szFile);
		return true;
	}
	return false;
}
bool JMemoryHistory::Delete(void* pPtr, bool bArray) {
	FILE* pLogFile;
	for (int iCnt = 0; iCnt < ALLOC_INFO_MAX; iCnt++) {
		if (_allocInfos[iCnt].pAlloc == pPtr) {
			// �迭�� �Ҵ� �� �Ϲ� ���� �� ����
			// �Ϲ� �Ҵ� �� �迭 ���� �� ����
			if (_allocInfos[iCnt].bArray != bArray) {
				errno_t err = fopen_s(&pLogFile, _logFileName, "a");
				if (err == 0) {
					// ���� ���� �� �α� ���Ͽ� �α�
					fprintf(pLogFile, "Array !! [0x%x] [%7d] \n",
						_allocInfos[iCnt].pAlloc, _allocInfos[iCnt].iSize);
					fclose(pLogFile);
				}
				return false;
			}
			_allocInfos[iCnt].pAlloc = NULL;
			return true;
		}
	}

	errno_t err = fopen_s(&pLogFile, _logFileName, "a");
	if (err != 0) {
		return false;
	}
	// �Ҵ�޾� ��ȯ�� �����ͷ� ������ �� NO ALLOC ������ �α�
	fprintf(pLogFile, "NO ALLOC [0x%x] \n", pPtr);
	fclose(pLogFile);

	return false;
}

////////////////////////////////////
// JMemoryHistory ���� ��ü
////////////////////////////////////
// JMemoryHistory ��ü�� �������� �����Ѵ�.
// �� ��ü�� �ܺ� cpp���� ���Ƿ� �����Ͽ� ����ϴ� ������ �ƴϴ�.
// ���� ��ü�μ� ���μ��� ���� �� ������, ���� �� �Ҹ��ڰ� �ڵ����� ȣ��Ǿ� �ʱ�ȭ, �α� �ڵ� ������ �����̴�.
JMemoryHistory g_jMemHistory;

////////////////////////////////////
// �����Լ��� new / delete �����ε�
////////////////////////////////////
// � Ŭ����, �ڷ����̵� ����� �� �ְ� ���� �Լ��� new�� �����ε��Ѵ�.
// �⺻ ���� ������ new / delete�� �ƴ� 'file ��'�� '����'�� ���ڷ� ���� new / delete �Լ��� �����ε��� �Ѵ�.

// new �������� ����: new �������� �޸� �Ҵ� -> Ŭ���� ������ ȣ�� + ���� �Լ� ���̺� ����
// new ������ �����ε� �� �츮�� �츮�� ���ϴ� ������� �޸� �Ҵ縸�� �ϰ�, Ŭ���� ������ ���õ� �۾��� �����Ϸ����� �ñ��.

void* operator new(size_t size, char* File, int Line) {
	void* p = malloc(size);
	g_jMemHistory.NewAlloc(p, File, Line, size);
	return p;
}
void* operator new[](size_t size, char* File, int Line) {
	void* p = malloc(size);
	g_jMemHistory.NewAlloc(p, File, Line, size, true);
	return p;
}
// ���� ���� �߰� ���ڰ� �����ϴ� new�� �����ε� �� ���
// ������ throw ���� �߻� �� �ϴ��� delete�� ȣ��Ǵ� ����ó���⿡�� ���� delete�� �����ε� ���־�� �Ѵ�.
// (���� ��������� ���� ���̳�, ��� ���ֱ� ���� �ܼ��� �����ε� �Ѵ�.)
void operator delete(void* p, char* File, int Line) {}
void operator delete[](void* p, char* File, int Line) {}

void operator delete(void* p) {
	if (g_jMemHistory.Delete(p)) {
		free(p);
	}
}
void operator delete[](void* p) {
	if (g_jMemHistory.Delete(p, true)) {
		free(p);
	}
}