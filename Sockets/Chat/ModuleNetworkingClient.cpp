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

		OutputMemoryStream packet;
		packet << ClientMessage::Hello;
		packet << playerName;

		if (sendPacket(packet, clientSocket))
		{			
		}
		else
		{
			ELOG("CLIENT ERROR: Error sending playerName to server");

			disconnect();
			state = ClientState::Stopped;
		}
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

		for (Message msg : messages)
		{
			if (!msg.server)
				ImGui::Text("%s: %s", msg.playerName.data(), msg.message.data());
			else
				ImGui::TextColored({ 1,1,0,1 }, "\"%s\".", msg.message.data());
		}

		//MESSAGE SENDING
		char message[1024] = "";
		ImVec2 windowPos = ImGui::GetWindowPos();
		ImGui::SetCursorScreenPos({ ImGui::GetCursorScreenPos().x, windowPos.y + ImGui::GetWindowHeight() - 30});
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Message:"); ImGui::SameLine();
		if (ImGui::InputText("##MessageInput", message, 1024, ImGuiInputTextFlags_::ImGuiInputTextFlags_EnterReturnsTrue))
		{
			Message msg;
			msg.message = message;
			msg.playerName = playerName;

			//SEND THE MESSAGE TO THE SERVER
			OutputMemoryStream packet;
			packet << ClientMessage::NewMessage;
			packet << msg.playerName;
			packet << msg.message;

			sendPacket(packet, clientSocket);

			//To keep the input text focused after sending the message
			ImGui::SetKeyboardFocusHere(-1);
		}

		ImGui::End();
	}

	return true;
}

void ModuleNetworkingClient::onSocketReceivedData(SOCKET socket, const InputMemoryStream& packet)
{
	//state = ClientState::Stopped;

	ServerMessage type;
	packet >> type;

	if (state == ClientState::Start)
	{
		if (type == ServerMessage::Welcome)
		{
			LOG("Welcome received from the server");
			state = ClientState::Logging;
		}
		else if (type == ServerMessage::PlayerNameUnavailable)
		{
			LOG("My name is not available");
			state = ClientState::Stopped;
			disconnect();
		}
	}
	else if (state == ClientState::Logging)
	{
		if (type == ServerMessage::NewMessage) 
		{
			Message msg;
			packet >> msg.playerName;
			packet >> msg.message;

			messages.push_back(msg);
		}
		else if (type == ServerMessage::ServerGlobalMessage)
		{
			Message msg;
			packet >> msg.message;
			
			msg.server = true;

			messages.push_back(msg);
		}
	}
	
}

void ModuleNetworkingClient::onSocketDisconnected(SOCKET socket)
{
	state = ClientState::Stopped;
}

