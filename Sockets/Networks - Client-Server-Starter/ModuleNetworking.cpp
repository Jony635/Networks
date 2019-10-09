#include "Networks.h"
#include "ModuleNetworking.h"

static uint8 NumModulesUsingWinsock = 0;

void ModuleNetworking::reportError(const char* inOperationDesc)
{
	LPVOID lpMsgBuf;
	DWORD errorNum = WSAGetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		errorNum,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	ELOG("Error %s: %d- %s", inOperationDesc, errorNum, lpMsgBuf);
}

void ModuleNetworking::disconnect()
{
	for (SOCKET socket : sockets)
	{
		shutdown(socket, 2);
		closesocket(socket);
	}

	sockets.clear();
}

bool ModuleNetworking::init()
{
	if (NumModulesUsingWinsock == 0)
	{
		NumModulesUsingWinsock++;

		WORD version = MAKEWORD(2, 2);
		WSADATA data;
		if (WSAStartup(version, &data) != 0)
		{
			reportError("ModuleNetworking::init() - WSAStartup");
			return false;
		}
	}

	return true;
}

bool ModuleNetworking::preUpdate()
{
	if (sockets.empty()) return true;

	// NOTE(jesus): You can use this temporary buffer to store data from recv()
	const uint32 incomingDataBufferSize = Kilobytes(1);
	byte incomingDataBuffer[incomingDataBufferSize];

	// TODO(jesus): select those sockets that have a read operation available

	fd_set socketSet;
	socketSet.fd_count = sockets.size();
	memcpy(socketSet.fd_array, &sockets[0], sockets.size() * sizeof(SOCKET));

	TIMEVAL timeOut;
	timeOut.tv_sec = 0;
	timeOut.tv_usec = 0;

	if (select(0, &socketSet, nullptr, nullptr, &timeOut) == SOCKET_ERROR)
	{
		ELOG("NETWORKING ERROR: Error selecting readable sockets.");
		return false;
	}

	for (int i = 0; i < socketSet.fd_count; ++i)
	{
		SOCKET socket = socketSet.fd_array[i];

		// TODO(jesus): for those sockets selected, check whether or not they are
		// a listen socket or a standard socket and perform the corresponding
		// operation (accept() an incoming connection or recv() incoming data,
		// respectively).
		// On accept() success, communicate the new connected socket to the
		// subclass (use the callback onSocketConnected()), and add the new
		// connected socket to the managed list of sockets.
		// On recv() success, communicate the incoming data received to the
		// subclass (use the callback onSocketReceivedData()).

		if (isListenSocket(socket))
		{
			//Accept new connections

			sockaddr_in clientAddr;
			int clientAddrSize = sizeof(clientAddr);

			SOCKET connected = accept(socket, (sockaddr*)&clientAddr, &clientAddrSize);

			if (connected == INVALID_SOCKET)
			{
				int code = WSAGetLastError();

				ELOG("NETWORKING ERROR: Error receiving client connection");
				return false;
			}

			onSocketConnected(connected, clientAddr);
			sockets.push_back(connected);
		}
		else
		{
			//Receive data from the connected server

			// TODO(jesus): handle disconnections. Remember that a socket has been
			// disconnected from its remote end either when recv() returned 0,
			// or when it generated some errors such as ECONNRESET.
			// Communicate detected disconnections to the subclass using the callback
			// onSocketDisconnected().

			// TODO(jesus): Finally, remove all disconnected sockets from the list
			// of managed sockets.

			int result = recv(socket, (char*)incomingDataBuffer, incomingDataBufferSize, 0);
			if (result == SOCKET_ERROR)
			{
				int code = WSAGetLastError();

				ELOG("NETWORKING ERROR: Error receiving connected server messages");
				onSocketDisconnected(socket);

				//Clean disconnected socket
				for (std::vector<SOCKET>::iterator it = sockets.begin(); it != sockets.end(); ++it)
				{
					if ((*it) == socket)
					{
						sockets.erase(it);
						break;
					}
				}

				return false;
			}
			else if (result == 0)
			{
				onSocketDisconnected(socket);

				//Clean disconnected socket
				for (std::vector<SOCKET>::iterator it = sockets.begin(); it != sockets.end(); ++it)
				{
					if ((*it) == socket)
					{
						sockets.erase(it);
						break;
					}
				}
			}
			else
			{
				onSocketReceivedData(socket, incomingDataBuffer);
			}		
		}
	}

	return true;
}

bool ModuleNetworking::cleanUp()
{
	disconnect();

	NumModulesUsingWinsock--;
	if (NumModulesUsingWinsock == 0)
	{

		if (WSACleanup() != 0)
		{
			reportError("ModuleNetworking::cleanUp() - WSACleanup");
			return false;
		}
	}

	return true;
}

void ModuleNetworking::addSocket(SOCKET socket)
{
	sockets.push_back(socket);
}
