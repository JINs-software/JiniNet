#pragma once

/**********************************/
/*
* [메모리 할당, 해제 정보를 저장해주는 클래스]
* 
* 실제 라이브 프로젝트에 사용하기 위해선 몇 가지의 추가 개선이 필요
* => 1. 메모리 할당 정보 저장을 배열에서 리스트 방식으로
* => 2. 클래스에 대한 배열 생성 시 4, 8 바이트를 더 확보하므로 내부에서 저장한 할당 포인터와
*		사용자에게 반환된 포인터는 다를 수 있음. (이에 대한 처리는 생략)
*/

class JMemoryHistory
{
public:
	enum {
		LOF_FILE_NAME_LEN = 64,
		FILE_NAME_LEN = 128,
		ALLOC_INFO_MAX = 1000
	};

	// 메모리 할당 정보
	struct stALLOC_INFO {
		void* pAlloc;				// 할당받은 포인터
		int iSize;					// 할당 사이즈
		char szFile[FILE_NAME_LEN];	// 할당 파일 위치
		int iLine;					// 파일 라인
		bool bArray;				// 배열 생성 여부
	};

private:
	// 메모리 할당 정보들 저장 배열
	stALLOC_INFO _allocInfos[ALLOC_INFO_MAX];
	char _logFileName[LOF_FILE_NAME_LEN];		// 로그 파일 이름을 저장할 배열

public:
	JMemoryHistory(const char* szLogFile = "AllocInfo");
	~JMemoryHistory();

private:
	bool SaveLogFile(void);
	bool NewAlloc(void* pPtr, char* szFile, int iLine, int iSize, bool bArray = false);
	bool Delete(void* pPtr, bool bArray = false);

	// new 연산자 오버로딩 함수에서는 본 클래스를 접근할 수 있다.
	friend void* operator new(size_t size, char* File, int Line);
	friend void* operator new[](size_t size, char* File, int Line);
	friend void operator delete(void* p);
	friend void operator delete[](void* p);
};

