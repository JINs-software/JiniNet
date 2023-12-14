#pragma once

//typedef unsigned char HostID;
//typedef unsigned char RpcID;
// TO DO: 현재 FightGame의 one-way를 맞추어 HostID와 RpcID의 자료형이 BYTE임
// 기존적으로 JNetRPC의 HostID와 RpcID의 자료형은 UINT와 USHORT로 선언
// 그리고 FightGame과 같이 자체 헤더가 정의된 one-way이 방식에선 json에 자체 헤더를 선언하고,
// RPC 함수에 buff << 헤더는 생성되지 않게 함.
// 즉, one-way에서는 자체 헤더가 RPC의 파라미터가 되도록...

typedef unsigned int HostID;
typedef unsigned short RpcID;