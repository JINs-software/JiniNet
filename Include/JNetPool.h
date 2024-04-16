#pragma once
//#include <stdlib.h>
//#include <mutex>
//
//typedef unsigned char BYTE;
//typedef uint32_t uint32;
//typedef uint64_t uint64;
//
//class JiniPool {
//public:
//	JiniPool() {};
//	JiniPool(size_t unitSize, size_t unitCnt)
//		: _unitSize(unitSize), _unitCnt(unitCnt)
//	{
//		listFront = freeListFront = (BYTE*)malloc((unitSize + sizeof(BYTE*)) * unitCnt);
//		BYTE* memPtr = freeListFront;
//		for (size_t i = 0; i < unitCnt; i++) {
//			memPtr += unitSize;
//			if (memPtr != NULL) {
//				if (i != unitCnt - 1) {
//					*(reinterpret_cast<uint64*>(memPtr)) = reinterpret_cast<uint64>(memPtr + sizeof(BYTE*));
//				}
//				else {
//					*(reinterpret_cast<uint64*>(memPtr)) = NULL;
//				}
//			}
//			memPtr += sizeof(BYTE*);
//		}
//	}
//	~JiniPool() {
//		free((void*)listFront);
//	}
//
//	// 할당
//	BYTE* AllocMem() {
//#if defined(THREAD_SAFE)
//		std::lock_guard<std::mutex> lock(_mutex);
//#endif
//		BYTE* retPtr = NULL;
//
//		if(freeListFront != NULL) {
//			retPtr = freeListFront;
//			freeListFront = reinterpret_cast<BYTE*>(*reinterpret_cast<uint64*>(freeListFront + _unitSize));
//			//return retPtr;
//		}
//		return retPtr;
//	}
//	// 반환
//	void ReturnMem(BYTE* ptr) {
//#if defined(THREAD_SAFE)
//		std::lock_guard<std::mutex> lock(_mutex);
//#endif
//		if (listFront <= ptr && ptr <= listFront + (_unitSize + sizeof(BYTE*)) * _unitCnt) {
//			*reinterpret_cast<uint64*>(ptr + _unitSize) = reinterpret_cast<uint64>(freeListFront);
//			freeListFront = ptr;
//		}
//	}
//
//private:
//	BYTE* listFront;
//	BYTE* freeListFront;
//
//	size_t _unitSize;
//	size_t _unitCnt;
//
//	std::mutex _mutex;
//};

#include <Windows.h>

class JiniPool {
public:
	JiniPool(size_t unitSize, size_t unitCnt) 
		: m_UnitSize(unitSize), m_UnitCnt(unitCnt)
	{
		m_ListFront = m_FreeFront = (PBYTE)malloc((m_UnitSize + sizeof(UINT_PTR)) * m_UnitCnt);
		PBYTE ptr = m_FreeFront;
		for (size_t idx = 0; idx < m_UnitCnt - 1; idx++) {
			ptr += m_UnitSize;
			assert(ptr != NULL);
			*(PUINT_PTR)ptr = (UINT_PTR)(ptr + sizeof(UINT_PTR));
			ptr += sizeof(UINT_PTR);
		}

		ptr += m_UnitSize;
		assert(ptr != NULL);
		*(PUINT_PTR)ptr = NULL;
	}
	~JiniPool() {
		free(m_ListFront);
	}

	BYTE* AllocMem() {
		PBYTE ret = NULL;
		if (m_FreeFront != NULL) {
			ret = m_FreeFront;
			m_FreeFront = (PBYTE)(*(PUINT_PTR)(m_FreeFront + m_UnitSize));
		}
		return ret;
	}
	void ReturnMem(BYTE* address) {
		PBYTE ptr = (PBYTE)address;
		ptr += m_UnitSize;
		*(PUINT_PTR)ptr = (UINT_PTR)m_FreeFront;
		m_FreeFront = (PBYTE)address;
	}

private:
	PBYTE m_ListFront;
	PBYTE m_FreeFront;

	size_t m_UnitSize;
	size_t m_UnitCnt;
};