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

	// YYYYMMDD_hhmmss 형식으로 변경
	sprintf_s(szTime, 17, "%04d%02d%02d_%02d%02d%02d",
		TM.tm_year + 1900,
		TM.tm_mon + 1,
		TM.tm_mday,
		TM.tm_hour,
		TM.tm_min,
		TM.tm_sec);

	// 로그 파일 이름: AllocInfo20240125_123045.txt
	strcat_s(_logFileName, LOF_FILE_NAME_LEN, szLogFile);
	strcat_s(_logFileName, LOF_FILE_NAME_LEN, szTime);
	strcat_s(_logFileName, LOF_FILE_NAME_LEN, ".txt");
}
JMemoryHistory::~JMemoryHistory() {
	SaveLogFile();
}

bool JMemoryHistory::SaveLogFile(void) {
	FILE* pLogFile;
	// 로그 파일 명의 파일 open
	errno_t err = fopen_s(&pLogFile, _logFileName, "a");	// 옵션 "a": 동일명 파일 존재시 끝에 이어씀, 동일명 파일 없으면 쓰기용 파일 생성
	if (err != 0) {
		return false;
	}

	// 로그 파일에 작성 누수 메모리 정보 출력
	for (int iCnt = 0; iCnt < ALLOC_INFO_MAX; iCnt++) {
		if (_allocInfos[iCnt].pAlloc != NULL) {
			// 저장 포멧
			// 해제된 메모리 - [메모리 포인터] [사이즈] 파일 위치 : 줄 번호
			// 누수된 메모리 - [메모리 포인터] [사이즈] 파일 위치 : 줄 번호
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
			// 배열로 할당 후 일반 해제 시 오류
			// 일반 할당 후 배열 해제 시 오류
			if (_allocInfos[iCnt].bArray != bArray) {
				errno_t err = fopen_s(&pLogFile, _logFileName, "a");
				if (err == 0) {
					// 해제 오류 시 로그 파일에 로깅
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
	// 할당받아 반환된 포인터로 해제할 시 NO ALLOC 정보를 로깅
	fprintf(pLogFile, "NO ALLOC [0x%x] \n", pPtr);
	fclose(pLogFile);

	return false;
}

////////////////////////////////////
// JMemoryHistory 전역 객체
////////////////////////////////////
// JMemoryHistory 객체를 전역으로 선언한다.
// 이 객체는 외부 cpp에서 임의로 선언하여 사용하는 목적이 아니다.
// 전역 객체로서 프로세스 가동 시 생성자, 종료 시 소멸자가 자동으로 호출되어 초기화, 로그 자동 저장이 목적이다.
JMemoryHistory g_jMemHistory;

////////////////////////////////////
// 전역함수로 new / delete 오버로딩
////////////////////////////////////
// 어떤 클래스, 자료형이든 적용될 수 있게 전역 함수로 new를 오버로딩한다.
// 기본 인자 형태의 new / delete가 아닌 'file 명'과 '라인'을 인자로 갖는 new / delete 함수로 오버로딩을 한다.

// new 연산자의 동작: new 연산자의 메모리 할당 -> 클래스 생성자 호출 + 가상 함수 테이블 셋팅
// new 연산자 오버로딩 시 우리는 우리는 원하는 사이즈로 메모리 할당만을 하고, 클래스 생성과 관련된 작업은 컴파일러에게 맡긴다.

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
// 위와 같이 추가 인자가 존재하는 new를 오버로딩 할 경우
// 생성자 throw 예외 발생 시 하단의 delete가 호출되는 예외처리기에서 사용될 delete를 오버로딩 해주어야 한다.
// (직접 사용하지는 않을 것이나, 경고를 없애기 위해 단순히 오버로딩 한다.)
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