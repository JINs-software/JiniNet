#pragma once
#include <stdlib.h>
//#include "Types.h"
#include <mutex>

typedef unsigned char BYTE;
typedef uint32_t uint32;
typedef uint64_t uint64;

//class JiniHeap {
//public:
//	void CreateHeap() {
//		HeapCreate(HEAP_NO_SERIALIZE, )
//	}
//};

class JiniPool {
public:
	JiniPool() {};
	//JiniPool(uint32 unitSize, uint32 unitCnt) 
	//	: _unitSize(unitSize), _unitCnt(unitCnt)
	//{
	//	listFront = freeListFront = (BYTE*)malloc((unitSize + sizeof(BYTE*))* unitCnt);
	//	BYTE* memPtr = freeListFront;
	//	for (uint32 i = 0; i < unitCnt; i++) {
	//		memPtr += unitSize;
	//		if (memPtr != NULL) {
	//			if (i != unitCnt - 1) {
	//				*(reinterpret_cast<uint64*>(memPtr)) = reinterpret_cast<uint64>(memPtr + sizeof(BYTE*));
	//			}
	//			else {
	//				*(reinterpret_cast<uint64*>(memPtr)) = NULL;
	//			}
	//		}
	//		memPtr += sizeof(BYTE*);
	//	}
	//}
	JiniPool(size_t unitSize, size_t unitCnt)
		: _unitSize(unitSize), _unitCnt(unitCnt)
	{
		listFront = freeListFront = (BYTE*)malloc((unitSize + sizeof(BYTE*)) * unitCnt);
		BYTE* memPtr = freeListFront;
		for (size_t i = 0; i < unitCnt; i++) {
			memPtr += unitSize;
			if (memPtr != NULL) {
				if (i != unitCnt - 1) {
					*(reinterpret_cast<uint64*>(memPtr)) = reinterpret_cast<uint64>(memPtr + sizeof(BYTE*));
				}
				else {
					*(reinterpret_cast<uint64*>(memPtr)) = NULL;
				}
			}
			memPtr += sizeof(BYTE*);
		}
	}
	~JiniPool() {
		free((void*)listFront);
	}

	// 할당
	BYTE* AllocMem() {
		std::lock_guard<std::mutex> lock(_mutex);
		BYTE* retPtr = NULL;
		// 조건 체크 ..
		//if (freeListFront == NULL) {
		//	return NULL;
		//}
		//else {
		if(freeListFront != NULL) {
			retPtr = freeListFront;
			freeListFront = reinterpret_cast<BYTE*>(*reinterpret_cast<uint64*>(freeListFront + _unitSize));
			//return retPtr;
		}
		return retPtr;
	}
	// 반환
	void ReturnMem(BYTE* ptr) {
		std::lock_guard<std::mutex> lock(_mutex);
		*reinterpret_cast<uint64*>(ptr + _unitSize) = reinterpret_cast<uint64>(freeListFront);
		freeListFront = ptr;
	}

private:
	BYTE* listFront;
	BYTE* freeListFront;
	//BYTE* freeListEnd;

	size_t _unitSize;
	size_t _unitCnt;

	std::mutex _mutex;
};