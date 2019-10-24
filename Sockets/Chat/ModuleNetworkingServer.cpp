#include "ModuleNetworkingServer.h"

//////////////////////////////////////////////////////////////////////
// ModuleNetworkingServer public methods
//////////////////////////////////////////////////////////////////////

bool ModuleNetworkingServer::start(int port)
{
	// TODO(jesus): TCP listen socket stuff

	// - Create the listenSocket
	listenSocket = socket(AF_INET, SOCK_STREAM, 0);

	// - Set address reuse
	bool enable = true;
	int result = setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)& enable, sizeof(enable));
	if (result != NO_ERROR)
	{
		ELOG("SERVER ERROR: Could not enable addresses reuse.");
		return false;
	}

	// - Bind the socket to a local interface
	sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.S_un.S_addr = INADDR_ANY;

	bind(listenSocket, (const sockaddr*)&address, sizeof(address));

	// - Enter in listen mode
	listen(listenSocket, SOMAXCONN);

	// - Add the listenSocket to the managed list of sockets using addSocket()
	addSocket(listenSocket);

	state = ServerState::Listening;

	return true;
}

bool ModuleNetworkingServer::isRunning() const
{
	return state != ServerState::Stopped;
}



//////////////////////////////////////////////////////////////////////
// Module virtual methods
//////////////////////////////////////////////////////////////////////

bool ModuleNetworkingServer::update()
{
	return true;
}

bool ModuleNetworkingServer::gui()
{
	if (state != ServerState::Stopped)
	{
		// NOTE(jesus): You can put ImGui code here for debugging purposes
		ImGui::Begin("Server Window");

		Texture *tex = App->modResources->server;
		ImVec2 texSize(400.0f, 400.0f * tex->height / tex->width);
		ImGui::Image(tex->shaderResource, texSize);

		ImGui::Text("List of connected sockets:");

		for (auto &connectedSocket : connectedSockets)
		{
			ImGui::Separator();
			ImGui::Text("Socket ID: %d", connectedSocket.socket);
			ImGui::Text("Address: %d.%d.%d.%d:%d",
				connectedSocket.address.sin_addr.S_un.S_un_b.s_b1,
				connectedSocket.address.sin_addr.S_un.S_un_b.s_b2,
				connectedSocket.address.sin_addr.S_un.S_un_b.s_b3,
				connectedSocket.address.sin_addr.S_un.S_un_b.s_b4,
				ntohs(connectedSocket.address.sin_port));
			ImGui::Text("Player name: %s", connectedSocket.playerName.c_str());
		}

		ImGui::End();
	}

	return true;
}



//////////////////////////////////////////////////////////////////////
// ModuleNetworking virtual methods
//////////////////////////////////////////////////////////////////////

bool ModuleNetworkingServer::isListenSocket(SOCKET socket) const
{
	return socket == listenSocket;
}

void ModuleNetworkingServer::onSocketConnected(SOCKET socket, const sockaddr_in &socketAddress)
{
	// Add a new connected socket to the list
	ConnectedSocket connectedSocket;
	connectedSocket.socket = socket;
	connectedSocket.address = socketAddress;
	connectedSockets.push_back(connectedSocket);
}

void ModuleNetworkingServer::onSocketReceivedData(SOCKET socket, const InputMemoryStream& packet)
{
	// Set the player name of the corresponding connected socket proxy

	ClientMessage clientMessage;
	packet >> clientMessage;

	if (clientMessage == ClientMessage::Hello)
	{
		std::string playerName;
		packet >> playerName;
		
		for (auto& connectedSocket : connectedSockets)
		{
			if (connectedSocket.playerName == playerName)
			{
				//Send PlayerNameUnavailable packet
				OutputMemoryStream outPacket;
				outPacket << ServerMessage::PlayerNameUnavailable;

				if (!sendPacket(outPacket, socket))
				{
					ELOG("SERVER ERROR: ERROR SENDING PlayerNameUnavailable MESSAGE TO THE CONNECTED CLIENT");
				}

				return;
			}
		}

		for (auto& connectedSocket : connectedSockets)
		{
			if (connectedSocket.socket == socket)
			{
				connectedSocket.playerName = playerName;

				//Notify everyone a new client has been connected
				OutputMemoryStream outPacketConnection;
				outPacketConnection << ServerMessage::ServerGlobalMessage;
				outPacketConnection << playerName + " joined";

				for (auto& connectedSocketB : connectedSockets)
				{
					if (connectedSocketB.socket == socket)
						continue;

					if (!sendPacket(outPacketConnection, connectedSocketB.socket))
					{
						ELOG("SERVER ERROR: ERROR SENDING USER CONNECTED MESSAGE TO THE CONNECTED CLIENTS");
					}
				}
				
				//Send welcome packet
				OutputMemoryStream outPacket;
				outPacket << ServerMessage::Welcome;

				if (!sendPacket(outPacket, socket))
				{
					ELOG("SERVER ERROR: ERROR SENDING WELCOME MESSAGE TO THE CONNECTED CLIENT");
				}
			}
		}
		connectedSockets;
	}
	else if (clientMessage == ClientMessage::NewMessage)
	{
		Message msg;
		packet >> msg.playerName;
		packet >> msg.message;

		if (msg.message[0] != '/')
		{
			// Send NewMessage Packet to all the connected sockets
			OutputMemoryStream outPacket;
			outPacket << ServerMessage::NewMessage;
			outPacket << msg.playerName;
			outPacket << msg.message;
			outPacket << msg.whispered;

			for (auto& connectedSocket : connectedSockets)
			{
				if (!sendPacket(outPacket, connectedSocket.socket))
				{
					ELOG("SERVER ERROR: ERROR SENDING NEW MESSAGE TO THE CONNECTED CLIENT");
				}
			}
		}
		else
		{
			//TODO: IMPLEMENT SERVER COMMANDS HERE

			if (msg.message == "/help")
			{
				OutputMemoryStream outPacket;
				outPacket << ServerMessage::ServerGlobalMessage;
				outPacket << "Available Commands:\n/list\n/kick playerName\n/whisper playerName message";

				if (!sendPacket(outPacket, socket))
				{
					ELOG("SERVER ERROR: ERROR SENDING /HELP RESPONSE TO THE CONNECTED CLIENT");
				}
			}
			else if (msg.message == "/list")
			{
				OutputMemoryStream outPacket;
				outPacket << ServerMessage::ServerGlobalMessage;

				std::string message = "Connected Users:";
				for (auto& connectedSocket : connectedSockets)
				{
					message += "\n- " + connectedSocket.playerName;
				}
				outPacket << message;
				
				if (!sendPacket(outPacket, socket))
				{
					ELOG("SERVER ERROR: ERROR SENDING /LIST RESPONSE TO THE CONNECTED CLIENT");
				}
			}
			else if (msg.message.find("/kick") != std::string::npos)
			{
				std::string playerName = msg.message.substr(msg.message.find("/kick") + 6);

				for (auto& connectedSocket : connectedSockets)
				{
					if (connectedSocket.playerName == playerName)
					{
						OutputMemoryStream outPacket;
						outPacket << ServerMessage::Disconnected;

						if (!sendPacket(outPacket, connectedSocket.socket))
						{
							ELOG("SERVER ERROR: ERROR SENDING /KICK RESPONSE TO THE CONNECTED CLIENT");
						}

						onSocketDisconnected(connectedSocket.socket);
					}
				}
			}
			else if (msg.message.find("/whisper") != std::string::npos)
			{
				std::string nameAndMessage = msg.message.substr(msg.message.find("/whisper") + 9);

				int nameEnd = nameAndMessage.find(" ");
				if (nameEnd != std::string::npos)
				{
					std::string name = nameAndMessage.substr(0, nameEnd);
					std::string message = nameAndMessage.substr(nameEnd+1);

					bool found = false;
					for (auto& connectedSocket : connectedSockets)
					{
						if (connectedSocket.playerName == name)
						{
							found = true;

							OutputMemoryStream outPacket;
							outPacket << ServerMessage::NewMessage;
							outPacket << msg.playerName;
							outPacket << message;
							outPacket << true;

							if (!sendPacket(outPacket, socket))
							{
								ELOG("SERVER ERROR: ERROR SENDING /WHISPER RESPONSE TO THE CONNECTED CLIENT");
							}

							if (connectedSocket.socket != socket)
							{
								if (!sendPacket(outPacket, connectedSocket.socket))
								{
									ELOG("SERVER ERROR: ERROR SENDING /WHISPER RESPONSE TO THE CONNECTED CLIENT");
								}
							}
						}
					}

					if (!found)
					{
						OutputMemoryStream outPacket;
						outPacket << ServerMessage::ServerGlobalMessage;
						outPacket << "USER NOT FOUND";

						if (!sendPacket(outPacket, socket))
						{
							ELOG("SERVER ERROR: ERROR SENDING /WHISPER RESPONSE TO THE CONNECTED CLIENT");
						}
					}

				}

			}
			else
			{
				OutputMemoryStream outPacket;
				outPacket << ServerMessage::ServerGlobalMessage;
				outPacket << msg.message + " is not an available command";

				if (!sendPacket(outPacket, socket))
				{
					ELOG("SERVER ERROR: ERROR SENDING COMMAND ERROR MESSAGE TO THE CONNECTED CLIENT");
				}
			}			
		}
	}
}

void ModuleNetworkingServer::onSocketDisconnected(SOCKET socket)
{
	// Remove the connected socket from the list
	for (auto it = connectedSockets.begin(); it != connectedSockets.end(); ++it)
	{
		auto &connectedSocket = *it;
		if (connectedSocket.socket == socket)
		{
			//Notify everyone client has been disconnected
			OutputMemoryStream outPacketConnection;
			outPacketConnection << ServerMessage::ServerGlobalMessage;
			outPacketConnection << connectedSocket.playerName + " left";

			for (auto& connectedSocketB : connectedSockets)
			{
				if (connectedSocketB.socket == socket)
					continue;

				if (!sendPacket(outPacketConnection, connectedSocketB.socket))
				{
					ELOG("SERVER ERROR: ERROR SENDING USER CONNECTED MESSAGE TO THE CONNECTED CLIENTS");
				}
			}

			connectedSockets.erase(it);		

			return;
		}
	}
}

