#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "Windows.h"
#include "WinSock2.h"
#include "Ws2tcpip.h"

#include <iostream>
#include <cstdlib>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_PORT 8888

#define PAUSE_AND_EXIT() system("pause"); exit(-1)

void printWSErrorAndExit(const char *msg)
{
	wchar_t *s = NULL;
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&s, 0, NULL);
	fprintf(stderr, "%s: %S\n", msg, s);
	LocalFree(s);
	PAUSE_AND_EXIT();
}

void server(int port)
{
	// TODO-1: Winsock init
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
	{
		// Log and handle error

		printWSErrorAndExit("Initializing Winsock");
		return;
	}

	// TODO-2: Create socket (IPv4, datagrams, UDP
	SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);

	// TODO-3: Force address reuse
	int enable = 1;
	int res = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)& enable, sizeof(int));
	if (res == SOCKET_ERROR) 
	{ 
		// Log and handle error 
		printWSErrorAndExit("Reuse addresses");
	}

	sockaddr_in bindAddr;
	bindAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	bindAddr.sin_family = AF_INET;
	bindAddr.sin_port = htons(port);

	// TODO-4: Bind to a local address
	res = bind(s, (const sockaddr*)&bindAddr, sizeof(bindAddr));

	while (true)
	{
		// TODO-5:
		// - Receive 'ping' packet from a remote host
		// - Answer with a 'pong' packet
		// - Control errors in both cases

		char* receivedData = new char[10];
		int bindAddrSize = sizeof(bindAddr);
		res = recvfrom(s, receivedData, 10, 0, (sockaddr*)& bindAddr, &bindAddrSize);
		if (res < 1)
		{
			printWSErrorAndExit("Receive Ping");
		}

		int res = sendto(s, "Pong", strlen("Pong"), 0, (const sockaddr*)& bindAddr, sizeof(bindAddr));
		if (res < 1)
		{
			printWSErrorAndExit("Sending Pong");
		}	

		delete[] receivedData;
	}

	// TODO-6: Close socketçres = WSACleanup();
	res = closesocket(s);
	if (res != NO_ERROR)
	{
		printWSErrorAndExit("Closing Socket");
	}

	// TODO-7: Winsock shutdown
	res = WSACleanup();
	if (res != NO_ERROR)
	{
		// Log and handle error

		printWSErrorAndExit("Closing Winsock");
	}
}

int main(int argc, char **argv)
{
	server(SERVER_PORT);

	PAUSE_AND_EXIT();
}
