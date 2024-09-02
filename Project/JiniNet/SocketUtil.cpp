#include "SocketUtil.h"

using namespace std;

void HandleError(const char* cause)
{
	int32 errCode = ::WSAGetLastError();
	cout << "[ERR] " << cause << ", errcode: " << errCode << endl;
}

int InitWindowSocketLib(LPWSADATA pWsaData)
{
	// winsock.h 
	// Windows 소켓 함수들을 호출하기 전 첫 번째로 호출되어야 하는 초기화 함수
	// (ws2_32 라이브러리 초기화)
	// 또한 소켓 구현의 세부 정보를 담는 WSADATA 데이터 획득
	return ::WSAStartup(MAKEWORD(2, 2), pWsaData);
	// 성공 시 0 반환
	// 실패 시 오류 코드 반환
}
void CleanUpWindowSocketLib() {
	::WSACleanup();
}

SOCKET CreateWindowSocket_IPv4(bool isTCP)
{
	// winsock2.h
	// 소캣 생성
	//SOCKET WSAAPI socket([in] int af, [in] int type, [in] int protocol);
	// 1) 주소 패밀리 지정: IPv4, IPv6 ..
	// 2) 소켓 타입 지정: SOCK_STREAM(TCP), SOCK_DGRAM(UDP) ..
	// 3) 프로토콜 지정: ('0' 전달, 2번째 매개변수인 소켓 타입을 지정하면, 이에 맞는 프로토콜로 셋팅됨)
	// cf) 소문자로 되어있는 API는 리눅스 환경에서도 동작할 가능성이 높다

	SOCKET sock;	// typedef UINT_PTR        SOCKET;

	if (isTCP) {
		sock = ::socket(AF_INET, SOCK_STREAM, 0);	// SOCK_STREAM - TCP(자동 설정)
	}
	else {
		sock = ::socket(AF_INET, SOCK_DGRAM, 0);	// SOCK_DGRAM - UDP(자동 설정)
	}

	if (sock == INVALID_SOCKET) {
		HandleError("CreateWindowSocket_IPv4");
	}

	return sock;
}

BOOL DomainToIP(WCHAR* szDomain, IN_ADDR* pAddr)
{
	ADDRINFOW* pAddrInfo;
	SOCKADDR_IN* pSockAddr;
	if (GetAddrInfo(szDomain, L"0", NULL, &pAddrInfo) != 0) {
		return FALSE;
	}
	else {
		pSockAddr = (SOCKADDR_IN*)pAddrInfo->ai_addr;
		*pAddr = pSockAddr->sin_addr;
		FreeAddrInfo(pAddrInfo);

		return TRUE;
	}
}

/*****************************************************************
* [Client]
*****************************************************************/
SOCKADDR_IN CreateDestinationADDR_LoopBack()
{
	SOCKADDR_IN serverAddr;
	(void)memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	//serverAddr.sin_addr.S_un.S_addr = ::inet_addr("127.0.0.1");
	//                    // ULONG S_addr;
	//                    // 4바이트 정수로 IPv4 주소 표현
	// 위 방식은 구식적인 방법, deprecate API warnings 발생! 아래와 같이 설정
	::inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
	serverAddr.sin_port = ::htons(7777);
	// host to network short
	// 호스트의 리틀 앤디안을 네트워크의 빅 엔디안 표기로

	return serverAddr;
}

SOCKADDR_IN CreateDestinationADDR(const char* serverIP, uint16 serverPort)
{
	SOCKADDR_IN serverAddr;
	(void)memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	//serverAddr.sin_addr.S_un.S_addr = ::inet_addr("127.0.0.1");
	//                    // ULONG S_addr;
	//                    // 4바이트 정수로 IPv4 주소 표현
	// 위 방식은 구식적인 방법, deprecate API warnings 발생! 아래와 같이 설정
	::inet_pton(AF_INET, serverIP, &serverAddr.sin_addr);
	serverAddr.sin_port = ::htons(serverPort);
	// host to network short
	// 호스트의 리틀 앤디안을 네트워크의 빅 엔디안 표기로

	return serverAddr;
}

SOCKADDR_IN CreateDestinationADDRbyDomain(WCHAR* serverDomain, uint16 serverPort)
{
	SOCKADDR_IN serverAddr;
	IN_ADDR Addr;
	(void)memset(&serverAddr, 0, sizeof(serverAddr));

	DomainToIP(serverDomain, &Addr);

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr = Addr;
	serverAddr.sin_port = ::htons(serverPort);

	return serverAddr;
}

int ConnectSocket(SOCKET& sock, SOCKADDR_IN& serverAddr)
{
	int ret;
	// int WSAAPI connect([in] SOCKET s, [in] const sockaddr* name, [in] int namelen);
	// 1) 연결되지 않은 소켓을 식별하는 설명자
	// 2) 연결을 설정해야 하는 sockaddr 구조체에 대한 포인터
	// 3) name 매개변수가 가리키는 sockaddr 구조체의 길이
	while (1) {
		ret = ::connect(sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
		if (ret == SOCKET_ERROR) {
			int32 errCode = ::WSAGetLastError();
			cout << "[ERR]: " << errCode << endl;
			continue;
		}
		else {
			break;
		}
	}

	return ret;
}

bool ConnectSocketTry(SOCKET& sock, SOCKADDR_IN& serverAddr)
{
	int ret = ::connect(sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (ret == SOCKET_ERROR) {
		return false;
	}
	else {
		return true;
	}
}

/*****************************************************************
* [Server]
*****************************************************************/
SOCKADDR_IN CreateServerADDR()
{
	SOCKADDR_IN serverAddr;
	(void)memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;

	//serverAddr.sin_addr.S_un.S_addr = ::htonl(INADDR_ANY);
	//const char serverIP[] = "172.30.1.100";
	//::inet_pton(AF_INET, serverIP, &serverAddr.sin_addr);

	// ::htonl(INADDR_ANY);
	// 서버의 네트워크 카드가 여러 개인 경우 주소가 여러 개 존재
	// 이 때 하드코딩하여 서버의 주소를 고정시킬 경우 이 주소로만 연결될 것
	// 하지만 INADDR_ANY 인자를 전달하여 루프백 주소를 포함하여 연결될 수 있는
	// 모든 주소를 연결할 수 있게 만든다.
	// 루프백 주소도 포함된다. 

	serverAddr.sin_port = ::htons(7777);
	// host to network short
	// 호스트의 리틀 앤디안을 네트워크의 빅 엔디안 표기로

	return serverAddr;
}

SOCKADDR_IN CreateServerADDR(uint16 port)
{
	SOCKADDR_IN serverAddr;
	(void)memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;

	serverAddr.sin_addr.S_un.S_addr = ::htonl(INADDR_ANY);
	//const char serverIP[] = "172.30.1.100";
	//::inet_pton(AF_INET, serverIP, &serverAddr.sin_addr);

	// ::htonl(INADDR_ANY);
	// 서버의 네트워크 카드가 여러 개인 경우 주소가 여러 개 존재
	// 이 때 하드코딩하여 서버의 주소를 고정시킬 경우 이 주소로만 연결될 것
	// 하지만 INADDR_ANY 인자를 전달하여 루프백 주소를 포함하여 연결될 수 있는
	// 모든 주소를 연결할 수 있게 만든다.
	// 루프백 주소도 포함된다. 

	serverAddr.sin_port = ::htons(port);
	// host to network short
	// 호스트의 리틀 앤디안을 네트워크의 빅 엔디안 표기로

	return serverAddr;
}

SOCKADDR_IN CreateServerADDR(const char* serverIP, uint16 port)
{
	SOCKADDR_IN serverAddr;
	(void)memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;

	::inet_pton(AF_INET, serverIP, &serverAddr.sin_addr);

	// ::htonl(INADDR_ANY);
	// 서버의 네트워크 카드가 여러 개인 경우 주소가 여러 개 존재
	// 이 때 하드코딩하여 서버의 주소를 고정시킬 경우 이 주소로만 연결될 것
	// 하지만 INADDR_ANY 인자를 전달하여 루프백 주소를 포함하여 연결될 수 있는
	// 모든 주소를 연결할 수 있게 만든다.
	// 루프백 주소도 포함된다. 

	serverAddr.sin_port = ::htons(port);
	// host to network short
	// 호스트의 리틀 앤디안을 네트워크의 빅 엔디안 표기로

	return serverAddr;
}

int BindSocket(SOCKET& sock, SOCKADDR_IN& serverAddr)
{
	int ret;
	if (ret = ::bind(sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		HandleError("BindSocket(소켓에 IP주소(ex, 설정되지 않은 IP주소) 또는 Port번호(ex, 이미 다른 프로세스가 바인딩한 포트 번호)를 바인딩할 수 없습니다.");
	}

	return ret;
}

int ListenSocket(SOCKET& sock)
{
	int ret;
	if ((ret = ::listen(sock, 10)) == SOCKET_ERROR) {
		int32 errCode = ::WSAGetLastError();
		cout << "[ERR]: " << errCode << endl;
	}

	return ret;
}
int ListenSocket(SOCKET& sock, int backlog)
{
	int ret;
	if (ret = ::listen(sock, backlog) == SOCKET_ERROR) {
		int32 errCode = ::WSAGetLastError();
		cout << "[ERR]: " << errCode << endl;
	}

	return ret;
}

SOCKET AcceptSocket(SOCKET& sock, SOCKADDR_IN& clientAddr)
{
	int32 addrLen = sizeof(clientAddr);
	SOCKET clientSock = ::accept(sock, (SOCKADDR*)&clientAddr, &addrLen);
	if (clientSock == INVALID_SOCKET) {
		int32 errCode = ::WSAGetLastError();
		cout << "[ERR]: " << errCode << endl;
	}

	return clientSock;
}
