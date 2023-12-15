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

	// ���� ������� �뷮 ���.
	UINT	GetUseSize(void) const;

	// ���� ���ۿ� ���� �뷮 ���. 
	UINT	GetFreeSize(void) const;

	// WritePos �� ����Ÿ ����.
	// Parameters: (BYTE *)����Ÿ ������. (UINT)ũ��. 
	// Return: (UINT)���� ũ��.
	UINT	Enqueue(const BYTE* data, UINT uiSize);


	// ReadPos ���� ����Ÿ ������. ReadPos �̵�.
	// Parameters: (BYTE *)����Ÿ ������. (UINT)ũ��.
	// Return: (UINT)������ ũ��.
	UINT	Dequeue(BYTE* dest, UINT uiSize);

	// ReadPos ���� ����Ÿ �о��. ReadPos ����.
	// Parameters: (void *)����Ÿ ������. (uUINT32_t)ũ��.
	// Return: (UINT)������ ũ��.
	UINT	Peek(OUT BYTE* dest, UINT uiSize);	
	template<typename T>
	UINT	Peek(OUT T* dest) { return Peek(reinterpret_cast<BYTE*>(dest), sizeof(T)); }
	bool	Peek(UINT offset, OUT BYTE* dest, UINT uiSize);


	// ������ ��� ����Ÿ ����.
	// Parameters: ����.
	// Return: ����.
	void	ClearBuffer(void);

	// ���� �����ͷ� �ܺο��� �ѹ濡 �а�, �� �� �ִ� ����. (������ ���� ����)
	// ���� ť�� ������ ������ ���ܿ� �ִ� �����ʹ� �� -> ó������ ���ư���
	// 2���� �����͸� ��ų� ���� �� ����. �� �κп��� �������� ���� ���̸� �ǹ�
	// Parameters: ����.
	// Return: (UINT)��밡�� �뷮.
	UINT	GetDirectEnqueueSize(void);
	UINT	GetDirectDequeueSize(void);
	// ���ϴ� ���̸�ŭ �б���ġ ���� ���� / ���� ��ġ �̵�
	// Parameters: ����.
	// Return: (UINT)�̵�ũ��
	UINT	DirectMoveEnqueueOffset(UINT uiSize);
	UINT	DirectMoveDequeueOffset(UINT uiSize);

	// ������ Front ������ ����.
	// Parameters: ����.
	// Return: (BYTE *) ���� ������.
	BYTE* GetEnqueueBufferPtr(void);
	void* GetEnqueueBufferVoidPtr(void);

	// ������ RearPos ������ ����.
	// Parameters: ����.
	// Return: (BYTE *) ���� ������.
	BYTE* GetDequeueBufferPtr(void);
	void* GetDequeueBufferVoidPtr(void);

	// [����ȭ]
	// ���� ��ť (>>)
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
	// ���� ��ť (<<)
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
	// ���� ���� ����
	// => �� ���� Ư���� �������� �޸� �ּ� ���� ��ȯ�� ������ �� ����.
	// => ���� DirectReserve ��� �Լ��� �����Ѵ�. ����ϴ� ���� DirectEnqueueSize() �Լ��� ����
	//    ���ӵ� ���� ������ �ִ��� Ȯ���Ѵ�.
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
