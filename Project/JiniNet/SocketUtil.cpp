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
	// Windows ���� �Լ����� ȣ���ϱ� �� ù ��°�� ȣ��Ǿ�� �ϴ� �ʱ�ȭ �Լ�
	// (ws2_32 ���̺귯�� �ʱ�ȭ)
	// ���� ���� ������ ���� ������ ��� WSADATA ������ ȹ��
	return ::WSAStartup(MAKEWORD(2, 2), pWsaData);
	// ���� �� 0 ��ȯ
	// ���� �� ���� �ڵ� ��ȯ
}
void CleanUpWindowSocketLib() {
	::WSACleanup();
}

SOCKET CreateWindowSocket_IPv4(bool isTCP)
{
	// winsock2.h
	// ��Ĺ ����
	//SOCKET WSAAPI socket([in] int af, [in] int type, [in] int protocol);
	// 1) �ּ� �йи� ����: IPv4, IPv6 ..
	// 2) ���� Ÿ�� ����: SOCK_STREAM(TCP), SOCK_DGRAM(UDP) ..
	// 3) �������� ����: ('0' ����, 2��° �Ű������� ���� Ÿ���� �����ϸ�, �̿� �´� �������ݷ� ���õ�)
	// cf) �ҹ��ڷ� �Ǿ��ִ� API�� ������ ȯ�濡���� ������ ���ɼ��� ����

	SOCKET sock;	// typedef UINT_PTR        SOCKET;

	if (isTCP) {
		sock = ::socket(AF_INET, SOCK_STREAM, 0);	// SOCK_STREAM - TCP(�ڵ� ����)
	}
	else {
		sock = ::socket(AF_INET, SOCK_DGRAM, 0);	// SOCK_DGRAM - UDP(�ڵ� ����)
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
	//                    // 4����Ʈ ������ IPv4 �ּ� ǥ��
	// �� ����� �������� ���, deprecate API warnings �߻�! �Ʒ��� ���� ����
	::inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
	serverAddr.sin_port = ::htons(7777);
	// host to network short
	// ȣ��Ʈ�� ��Ʋ �ص���� ��Ʈ��ũ�� �� ����� ǥ���

	return serverAddr;
}

SOCKADDR_IN CreateDestinationADDR(const char* serverIP, uint16 serverPort)
{
	SOCKADDR_IN serverAddr;
	(void)memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	//serverAddr.sin_addr.S_un.S_addr = ::inet_addr("127.0.0.1");
	//                    // ULONG S_addr;
	//                    // 4����Ʈ ������ IPv4 �ּ� ǥ��
	// �� ����� �������� ���, deprecate API warnings �߻�! �Ʒ��� ���� ����
	::inet_pton(AF_INET, serverIP, &serverAddr.sin_addr);
	serverAddr.sin_port = ::htons(serverPort);
	// host to network short
	// ȣ��Ʈ�� ��Ʋ �ص���� ��Ʈ��ũ�� �� ����� ǥ���

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
	// 1) ������� ���� ������ �ĺ��ϴ� ������
	// 2) ������ �����ؾ� �ϴ� sockaddr ����ü�� ���� ������
	// 3) name �Ű������� ����Ű�� sockaddr ����ü�� ����
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
	// ������ ��Ʈ��ũ ī�尡 ���� ���� ��� �ּҰ� ���� �� ����
	// �� �� �ϵ��ڵ��Ͽ� ������ �ּҸ� ������ų ��� �� �ּҷθ� ����� ��
	// ������ INADDR_ANY ���ڸ� �����Ͽ� ������ �ּҸ� �����Ͽ� ����� �� �ִ�
	// ��� �ּҸ� ������ �� �ְ� �����.
	// ������ �ּҵ� ���Եȴ�. 

	serverAddr.sin_port = ::htons(7777);
	// host to network short
	// ȣ��Ʈ�� ��Ʋ �ص���� ��Ʈ��ũ�� �� ����� ǥ���

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
	// ������ ��Ʈ��ũ ī�尡 ���� ���� ��� �ּҰ� ���� �� ����
	// �� �� �ϵ��ڵ��Ͽ� ������ �ּҸ� ������ų ��� �� �ּҷθ� ����� ��
	// ������ INADDR_ANY ���ڸ� �����Ͽ� ������ �ּҸ� �����Ͽ� ����� �� �ִ�
	// ��� �ּҸ� ������ �� �ְ� �����.
	// ������ �ּҵ� ���Եȴ�. 

	serverAddr.sin_port = ::htons(port);
	// host to network short
	// ȣ��Ʈ�� ��Ʋ �ص���� ��Ʈ��ũ�� �� ����� ǥ���

	return serverAddr;
}

SOCKADDR_IN CreateServerADDR(const char* serverIP, uint16 port)
{
	SOCKADDR_IN serverAddr;
	(void)memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;

	::inet_pton(AF_INET, serverIP, &serverAddr.sin_addr);

	// ::htonl(INADDR_ANY);
	// ������ ��Ʈ��ũ ī�尡 ���� ���� ��� �ּҰ� ���� �� ����
	// �� �� �ϵ��ڵ��Ͽ� ������ �ּҸ� ������ų ��� �� �ּҷθ� ����� ��
	// ������ INADDR_ANY ���ڸ� �����Ͽ� ������ �ּҸ� �����Ͽ� ����� �� �ִ�
	// ��� �ּҸ� ������ �� �ְ� �����.
	// ������ �ּҵ� ���Եȴ�. 

	serverAddr.sin_port = ::htons(port);
	// host to network short
	// ȣ��Ʈ�� ��Ʋ �ص���� ��Ʈ��ũ�� �� ����� ǥ���

	return serverAddr;
}

int BindSocket(SOCKET& sock, SOCKADDR_IN& serverAddr)
{
	int ret;
	if (ret = ::bind(sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		HandleError("BindSocket(���Ͽ� IP�ּ�(ex, �������� ���� IP�ּ�) �Ǵ� Port��ȣ(ex, �̹� �ٸ� ���μ����� ���ε��� ��Ʈ ��ȣ)�� ���ε��� �� �����ϴ�.");
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
