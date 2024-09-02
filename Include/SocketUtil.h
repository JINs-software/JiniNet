#pragma once

#include "CommHdr.h"
#include "Types.h"
#include <WinSock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32")

void HandleError(const char* cause);

int InitWindowSocketLib(LPWSADATA);
void CleanUpWindowSocketLib();

SOCKET CreateWindowSocket_IPv4(bool isTCP);

// 도메인 -> IP 주소
BOOL DomainToIP(WCHAR* szDomain, IN_ADDR* pAddr);

// Client
SOCKADDR_IN CreateDestinationADDR_LoopBack();
SOCKADDR_IN CreateDestinationADDR(const char* serverIP, uint16 serverPort);
SOCKADDR_IN CreateDestinationADDRbyDomain(WCHAR* serverDomain, uint16 serverPort);
int ConnectSocket(SOCKET& sock, SOCKADDR_IN& serverAddr);
bool ConnectSocketTry(SOCKET& sock, SOCKADDR_IN& serverAddr);

// Server
SOCKADDR_IN CreateServerADDR();
SOCKADDR_IN CreateServerADDR(uint16 port);
SOCKADDR_IN CreateServerADDR(const char* serverIp, uint16 port);
int BindSocket(SOCKET& sock, SOCKADDR_IN& serverAddr);
int ListenSocket(SOCKET& sock);
int ListenSocket(SOCKET& sock, int backlog);
SOCKET AcceptSocket(SOCKET& sock, SOCKADDR_IN& clientAddr);