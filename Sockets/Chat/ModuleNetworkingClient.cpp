#include "ModuleNetworkingClient.h"

bool  ModuleNetworkingClient::start(const char * serverAddressStr, int serverPort, const char *pplayerName)
{
	playerName = pplayerName;

	// TODO(jesus): TCP connection stuff

	// - Create the socket
	clientSocket = socket(AF_INET, SOCK_STREAM, 0);

	// - Create the remote address object
	sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port = htons(serverPort);
	inet_pton(AF_INET, serverAddressStr, &address.sin_addr);

	// - Connect to the remote address
	int result = connect(clientSocket, (const sockaddr*)&address, sizeof(address));
	if (result != NO_ERROR)
	{
		ELOG("CLIENT ERROR: Could not connect to the server address.");

		return false;
	}
	else
	{
		// - Add the created socket to the managed list of sockets using addSocket()
		addSocket(clientSocket);

		// If everything was ok... change the state
		state = ClientState::Start;

		return true;
	}
}

bool ModuleNetworkingClient::isRunning() const
{
	return state != ClientState::Stopped;
}

bool ModuleNetworkingClient::update()
{
	if (state == ClientState::Start)
	{
		// TODO(jesus): Send the player name to the server

		if (send(clientSocket, playerName.c_str(), playerName.size() + 1, 0) == SOCKET_ERROR)
		{
			ELOG("CLIENT ERROR: Error sending playerName to server");
			return false;
		}
		
		state = ClientState::Logging;
	}

	return true;
}

bool ModuleNetworkingClient::gui()
{
	if (state != ClientState::Stopped)
	{
		// NOTE(jesus): You can put ImGui code here for debugging purposes
		ImGui::Begin("Client Window");

		Texture *tex = App->modResources->client;
		ImVec2 texSize(400.0f, 400.0f * tex->height / tex->width);
		ImGui::Image(tex->shaderResource, texSize);

		ImGui::Text("%s connected to the server...", playerName.c_str());

		ImGui::End();
	}

	return true;
}

void ModuleNetworkingClient::onSocketReceivedData(SOCKET socket, const InputMemoryStream& packet)
{
	state = ClientState::Stopped;
}

void ModuleNetworkingClient::onSocketDisconnected(SOCKET socket)
{
	state = ClientState::Stopped;
}

