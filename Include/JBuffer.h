#pragma once
#include <stdexcept>

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

typedef unsigned char       BYTE;
typedef int                 INT;
typedef unsigned int        UINT;

class JBuffer
{
private:
	UINT		enqOffset;
	UINT		deqOffset;
	BYTE*		buffer;
	UINT		capacity;

public:
	JBuffer(UINT capacity);
	~JBuffer();

	void Resize(UINT size);

	UINT	GetBufferSize(void) const;

	// 현재 사용중인 용량 얻기.
	UINT	GetUseSize(void) const;

	// 현재 버퍼에 남은 용량 얻기. 
	UINT	GetFreeSize(void) const;

	// WritePos 에 데이타 넣음.
	// Parameters: (BYTE *)데이타 포인터. (UINT)크기. 
	// Return: (UINT)넣은 크기.
	UINT	Enqueue(const BYTE* data, UINT uiSize);


	// ReadPos 에서 데이타 가져옴. ReadPos 이동.
	// Parameters: (BYTE *)데이타 포인터. (UINT)크기.
	// Return: (UINT)가져온 크기.
	UINT	Dequeue(BYTE* dest, UINT uiSize);

	// ReadPos 에서 데이타 읽어옴. ReadPos 고정.
	// Parameters: (void *)데이타 포인터. (uUINT32_t)크기.
	// Return: (UINT)가져온 크기.
	UINT	Peek(OUT BYTE* dest, UINT uiSize);	
	template<typename T>
	UINT	Peek(OUT T* dest) { return Peek(reinterpret_cast<BYTE*>(dest), sizeof(T)); }
	bool	Peek(UINT offset, OUT BYTE* dest, UINT uiSize);


	// 버퍼의 모든 데이타 삭제.
	// Parameters: 없음.
	// Return: 없음.
	void	ClearBuffer(void);

	// 버퍼 포인터로 외부에서 한방에 읽고, 쓸 수 있는 길이. (끊기지 않은 길이)
	// 원형 큐의 구조상 버퍼의 끝단에 있는 데이터는 끝 -> 처음으로 돌아가서
	// 2번에 데이터를 얻거나 넣을 수 있음. 이 부분에서 끊어지지 않은 길이를 의미
	// Parameters: 없음.
	// Return: (UINT)사용가능 용량.
	UINT	GetDirectEnqueueSize(void);
	UINT	GetDirectDequeueSize(void);
	// 원하는 길이만큼 읽기위치 에서 삭제 / 쓰기 위치 이동
	// Parameters: 없음.
	// Return: (UINT)이동크기
	UINT	DirectMoveEnqueueOffset(UINT uiSize);
	UINT	DirectMoveDequeueOffset(UINT uiSize);

	// 버퍼의 Front 포인터 얻음.
	// Parameters: 없음.
	// Return: (BYTE *) 버퍼 포인터.
	BYTE* GetEnqueueBufferPtr(void);
	void* GetEnqueueBufferVoidPtr(void);

	// 버퍼의 RearPos 포인터 얻음.
	// Parameters: 없음.
	// Return: (BYTE *) 버퍼 포인터.
	BYTE* GetDequeueBufferPtr(void);
	void* GetDequeueBufferVoidPtr(void);

	// [직렬화]
	// 버퍼 디큐 (>>)
	template<typename T>
	JBuffer& operator>>(OUT T& dest) {
		if (GetUseSize() >= sizeof(T)) {
			Dequeue(reinterpret_cast<BYTE*>(&dest), sizeof(T));
			return *this;
		}
		else {
			throw std::runtime_error("[ERR] Serialization error : Buffer contains data size smaller than the size of dest type!");
		}
	}
	// 버퍼 인큐 (<<)
	//JBuffer& operator<<(BYTE src);
	//template<typename T>
	//JBuffer& operator<<(T src) {
	//	if (GetFreeSize() >= sizeof(T)) {
	//		Enqueue(reinterpret_cast<const BYTE*>(&src), sizeof(T));
	//	}
	//	else {
	//		throw std::runtime_error("[ERR] Serialization error : Buffer is fulled!");
	//	}
	//}
	template<typename T>
	JBuffer& operator<<(const T& src) {
		if (GetFreeSize() >= sizeof(T)) {
			Enqueue(reinterpret_cast<const BYTE*>(&src), sizeof(T));
		}
		else {
			throw std::runtime_error("[ERR] Serialization error : Buffer is fulled!");
		}
	}
	// 버퍼 공간 예약
	// => 링 버퍼 특성상 연속적인 메모리 주소 공간 반환을 보장할 수 없다.
	// => 따라서 DirectReserve 라는 함수를 제공한다. 사용하는 측은 DirectEnqueueSize() 함수를 통해
	//    연속된 예약 공간이 있는지 확인한다.
	template<typename T>
	T* DirectReserve() {
		T* ret = nullptr;
		if (GetDirectEnqueueSize() >= sizeof(T)) {
			ret = reinterpret_cast<T*>(GetEnqueueBufferPtr());
			DirectMoveEnqueueOffset(sizeof(T));
		}
		return ret;
	}
};
