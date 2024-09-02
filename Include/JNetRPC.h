#pragma once

// FightGame�� ��Ŷ ������� ��Ŷ Ÿ�� �ڷ����� Byte, ����  RpcID�� �ڷ����� BYTE�� �Ǿ�� ��.
// ���� JPD �����Ϸ��� ���� ��Ŷ ����� �����ϰ�, �������� ũ���� ����� ������ �� �ֵ��� ��

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