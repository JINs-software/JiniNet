#pragma once

// FightGame의 패킷 사이즈와 패킷 타입 자료형은 Byte, 따라서  RpcID의 자료형이 BYTE가 되어야 함.
// 추후 JPD 컴파일러를 통해 패킷 헤더를 생성하고, 가변적인 크기의 헤더를 적용할 수 있도록 함

typedef unsigned char RpcID;

//#define HDR_UNIQUE_NUM_TYPE				unsigned char
//#define HDR_MSG_ID_TYPE					unsigned short
//#define HDR_MSG_LEN_TYPE				unsigned short

#pragma pack(push, 1)
struct stMSG_HDR {
	//HDR_UNIQUE_NUM_TYPE		uniqueNum;
	//HDR_MSG_ID_TYPE			msgID;
	//HDR_MSG_LEN_TYPE		msgLen;

	unsigned char			byCode;
	unsigned char			bySize;
	unsigned char			byType;
};
#pragma pack(pop)

#define UNIQUE_PACK_NUM 0x77