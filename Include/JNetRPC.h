#pragma once

//typedef unsigned char HostID;
//typedef unsigned char RpcID;
// TO DO: ���� FightGame�� one-way�� ���߾� HostID�� RpcID�� �ڷ����� BYTE��
// ���������� JNetRPC�� HostID�� RpcID�� �ڷ����� UINT�� USHORT�� ����
// �׸��� FightGame�� ���� ��ü ����� ���ǵ� one-way�� ��Ŀ��� json�� ��ü ����� �����ϰ�,
// RPC �Լ��� buff << ����� �������� �ʰ� ��.
// ��, one-way������ ��ü ����� RPC�� �Ķ���Ͱ� �ǵ���...

typedef unsigned short RpcID;
#define ONEWAY_RPCID 0

#define HDR_UNIQUE_NUM_TYPE				unsigned char
#define HDR_MSG_ID_TYPE					unsigned short
#define HDR_MSG_LEN_TYPE				unsigned short

struct stJMSG_HDR {
	HDR_UNIQUE_NUM_TYPE		uniqueNum;
	HDR_MSG_ID_TYPE			msgID;
	HDR_MSG_LEN_TYPE		msgLen;
};

#define UNIQUE_PACK_NUM 0x77