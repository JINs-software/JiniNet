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