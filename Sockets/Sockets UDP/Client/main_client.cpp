#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "Windows.h"
#include "WinSock2.h"
#include "Ws2tcpip.h"

#include <iostream>
#include <cstdlib>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_ADDRESS "127.0.0.1"

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

void client(const char *serverAddrStr, int port)
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

	// TODO-2: Create socket (IPv4, datagrams, UDP)
	SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);

	// TODO-3: Create an address object with the server address
	const char* ip = SERVER_ADDRESS;

	sockaddr_in remoteAddr;
	inet_pton(AF_INET, ip, &remoteAddr.sin_addr);
	remoteAddr.sin_family = AF_INET;
	remoteAddr.sin_port = htons(port);

	while (true)
	{
		// TODO-4:
		// - Send a 'ping' packet to the server
		// - Receive 'pong' packet from the server
		// - Control errors in both cases

		int res = sendto(s, "Ping", strlen("Ping"), 0, (const sockaddr*)&remoteAddr, sizeof(remoteAddr));

		std::cout << "Ping ";

		if(res < 1)
		{
			printWSErrorAndExit("Sending Ping");
		}

		char* receivedData = new char[10];
		int remoteAdrrSize = sizeof(remoteAddr);
		res = recvfrom(s, receivedData, 10, 0, (sockaddr*)&remoteAddr, &remoteAdrrSize);
		if (res < 1)
		{
			int error_code = WSAGetLastError();

			//Error 10054:
			//Se ha forzado la interrupci¾n de una conexi¾n existente por el host remoto.
			//Solo en mi casa, aquí no sucede.

			printWSErrorAndExit("Receive Pong");
		}

		delete[] receivedData;
	}

	// TODO-5: Close socket
	int res = closesocket(s);
	if (res != NO_ERROR)
	{
		printWSErrorAndExit("Closing Socket");
	}

	// TODO-6: Winsock shutdown
	res = WSACleanup();
	if (res != NO_ERROR)
	{
		// Log and handle error

		printWSErrorAndExit("Closing Winsock");
	}
}

int main(int argc, char **argv)
{
	client(SERVER_ADDRESS, SERVER_PORT);

	PAUSE_AND_EXIT();
}
