#include "Networks.h"
#include "ModuleNetworkingClient.h"



//////////////////////////////////////////////////////////////////////
// ModuleNetworkingClient public methods
//////////////////////////////////////////////////////////////////////


void ModuleNetworkingClient::setServerAddress(const char * pServerAddress, uint16 pServerPort)
{
	serverAddressStr = pServerAddress;
	serverPort = pServerPort;
}

void ModuleNetworkingClient::setPlayerInfo(const char * pPlayerName, uint8 pSpaceshipType)
{
	playerName = pPlayerName;
	spaceshipType = pSpaceshipType;
}



//////////////////////////////////////////////////////////////////////
// ModuleNetworking virtual methods
//////////////////////////////////////////////////////////////////////

void ModuleNetworkingClient::onStart()
{
	if (!createSocket()) return;

	if (!bindSocketToPort(0)) {
		disconnect();
		return;
	}

	// Create remote address
	serverAddress = {};
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(serverPort);
	int res = inet_pton(AF_INET, serverAddressStr.c_str(), &serverAddress.sin_addr);
	if (res == SOCKET_ERROR) {
		reportError("ModuleNetworkingClient::startClient() - inet_pton");
		disconnect();
		return;
	}

	state = ClientState::Start;

	inputDataFront = 0;
	inputDataBack = 0;

	secondsSinceLastInputDelivery = 0.0f;
	secondsSinceLastPing = 0.0f;
	lastPacketReceivedTime = Time.time;

	repManager = {};
	delManager = {};
}

void ModuleNetworkingClient::onGui()
{
	GameObject* playerGameObject = App->modLinkingContext->getNetworkGameObject(networkId);

	if (playerGameObject != nullptr && !playerGameObject->enabled) return;

	if (state == ClientState::Stopped) return;

	if (ImGui::CollapsingHeader("ModuleNetworkingClient", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (state == ClientState::WaitingWelcome)
		{
			ImGui::Text("Waiting for server response...");
		}
		else if (state == ClientState::Playing)
		{
			ImGui::Text("Connected to server");

			ImGui::Separator();

			ImGui::Text("Player info:");
			ImGui::Text(" - Id: %u", playerId);
			ImGui::Text(" - Name: %s", playerName.c_str());

			ImGui::Separator();

			ImGui::Text("Spaceship info:");
			ImGui::Text(" - Type: %u", spaceshipType);
			ImGui::Text(" - Network id: %u", networkId);

			vec2 playerPosition = {};
			GameObject *playerGameObject = App->modLinkingContext->getNetworkGameObject(networkId);
			if (playerGameObject != nullptr) {
				playerPosition = playerGameObject->position;
			}
			ImGui::Text(" - Coordinates: (%f, %f)", playerPosition.x, playerPosition.y);

			ImGui::Separator();

			ImGui::Text("Connection checking info:");
			ImGui::Text(" - Ping interval (s): %f", PING_INTERVAL_SECONDS);
			ImGui::Text(" - Disconnection timeout (s): %f", DISCONNECT_TIMEOUT_SECONDS);

			ImGui::Separator();

			ImGui::Text("Input:");
			ImGui::InputFloat("Delivery interval (s)", &inputDeliveryIntervalSeconds, 0.01f, 0.1f, 4);
		}
	}

	float windowWidth, windowHeight;
	App->modRender->getViewportSize(windowWidth, windowHeight);

	ImGui::SetNextWindowPos(ImVec2(windowWidth, 0), 0, ImVec2(1.0f, 0.0f));
	ImGui::SetNextWindowSize(ImVec2(windowWidth * 0.25, windowHeight * 0.25), ImGuiCond_::ImGuiCond_Always);
	ImGui::Begin("Score Rank", nullptr, ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize);

	if(!playerBeforeName.empty())
		ImGui::Text("Previous player: %s Points: %u", playerBeforeName.c_str(), playerBeforePoints);
	else
		ImGui::Text("Previous player: NONE");

	ImGui::Text("You: %s Points: %u", playerName.c_str(), points);

	if(!playerAfterName.empty())
		ImGui::Text("Following player: %s Points: %u", playerAfterName.c_str(), playerAfterPoints);
	else	
		ImGui::Text("Following player: NONE");


	ImGui::End();
}

void ModuleNetworkingClient::onPacketReceived(const InputMemoryStream &packet, const sockaddr_in &fromAddress)
{
	lastPacketReceivedTime = Time.time;

	ServerMessage message;
	packet >> message;

	if (state == ClientState::WaitingWelcome)
	{
		if (message == ServerMessage::Welcome)
		{
			packet >> playerId;
			packet >> networkId;

			LOG("ModuleNetworkingClient::onPacketReceived() - Welcome from server");
			state = ClientState::Playing;
		}
		else if (message == ServerMessage::Unwelcome)
		{
			WLOG("ModuleNetworkingClient::onPacketReceived() - Unwelcome from server :-(");
			disconnect();
		}
	}
	else if (state == ClientState::Playing)
	{
		//Handle incoming messages from servers
		if (message == ServerMessage::Replication)
		{
			if(delManager.processSequenceNumber(packet))
				repManager.read(packet);

			else //Empty the packet without reading the content
			{
				char temp;
				while (packet.RemainingByteCount() > sizeof(uint32))
					packet >> temp;
			}

			packet >> inputDataFront;

			//REAPPLY THE INPUTS NOT PROCESSED BY THE SERVER YET, AFTER THE REPLICATION PROCESS FINISHES.
			//The previous input processed by the server is inputDataFront - 1
			//from inputDataFront to inputDataBack apply all inputs to our gameObject.

			GameObject* playerGameObject = App->modLinkingContext->getNetworkGameObject(networkId);
			if (playerGameObject != nullptr && playerGameObject->enabled)
			{
				for (int i = inputDataFront; i != inputDataBack; ++i)
				{
					InputPacketData inputPacketData = inputData[i];
					InputController inputController = inputControllerFromInputPacketData(inputPacketData);

					playerGameObject->behaviour->onInput(Input);
				}
			}	
		}
		else if (message == ServerMessage::ScoreListUpdate)
		{
			packet >> playerBeforeName;
			packet >> playerBeforePoints;
			packet >> playerAfterName;
			packet >> playerAfterPoints;
		}
		else if (message == ServerMessage::EnemyKilled)
		{
			OnEnemyKilled();
		}
	}
}

void ModuleNetworkingClient::onUpdate()
{
	if (state == ClientState::Stopped) return;

	if (state == ClientState::Start)
	{
		// Send the hello packet with player data

		OutputMemoryStream stream;
		stream << ClientMessage::Hello;
		stream << playerName;
		stream << spaceshipType;

		sendPacket(stream, serverAddress);

		state = ClientState::WaitingWelcome;
	}
	else if (state == ClientState::WaitingWelcome)
	{	}
	else if (state == ClientState::Playing)
	{
		secondsSinceLastInputDelivery += Time.deltaTime;
		secondsSinceLastPing += Time.deltaTime;

		if (inputDataBack - inputDataFront < ArrayCount(inputData))
		{
			uint32 currentInputData = inputDataBack++;
			InputPacketData &inputPacketData = inputData[currentInputData % ArrayCount(inputData)];
			inputPacketData.sequenceNumber = currentInputData;
			inputPacketData.horizontalAxis = Input.horizontalAxis;
			inputPacketData.verticalAxis = Input.verticalAxis;
			inputPacketData.buttonBits = packInputControllerButtons(Input);

			//Process the new input: Client Side
			GameObject* playerGameObject = App->modLinkingContext->getNetworkGameObject(networkId);
			if (playerGameObject != nullptr && playerGameObject->enabled)
			{
				playerGameObject->behaviour->onInput(Input);
			}

			// Create packet (if there's input and the input delivery interval exceeded)
			if (secondsSinceLastInputDelivery > inputDeliveryIntervalSeconds)
			{
				secondsSinceLastInputDelivery = 0.0f;

				OutputMemoryStream packet;
				packet << ClientMessage::Input;

				for (uint32 i = inputDataFront; i < inputDataBack; ++i)
				{
					InputPacketData &inputPacketData = inputData[i % ArrayCount(inputData)];
					packet << inputPacketData.sequenceNumber;
					packet << inputPacketData.horizontalAxis;
					packet << inputPacketData.verticalAxis;
					packet << inputPacketData.buttonBits;
				}

				// Clear the queue
				//inputDataFront = inputDataBack;

				sendPacket(packet, serverAddress);
			}
		}

		//Disconnect the client if the time since the last received packet is greater than DISCONNECT_TIMEOUT_SECONDS
		if (Time.time - lastPacketReceivedTime >= DISCONNECT_TIMEOUT_SECONDS)
		{
			//Disconnect the client
			disconnect();
		}

		//Send regular pings to the server
		if (secondsSinceLastPing >= PING_INTERVAL_SECONDS)
		{
			secondsSinceLastPing = 0.0f;
			
			OutputMemoryStream packet;
			packet << ClientMessage::Ping;

			delManager.writeSequenceNumbersPendingAck(packet);		

			sendPacket(packet, serverAddress);
		}
	}

	// Make the camera focus the player game object
	GameObject *playerGameObject = App->modLinkingContext->getNetworkGameObject(networkId);
	if (playerGameObject != nullptr && playerGameObject->enabled)
	{
		App->modRender->cameraPosition = playerGameObject->position;
	}
}

void ModuleNetworkingClient::onConnectionReset(const sockaddr_in & fromAddress)
{
	disconnect();
}

void ModuleNetworkingClient::onDisconnect()
{
	state = ClientState::Stopped;

	// Get all network objects and clear the linking context
	uint16 networkGameObjectsCount;
	GameObject *networkGameObjects[MAX_NETWORK_OBJECTS] = {};
	App->modLinkingContext->getNetworkGameObjects(networkGameObjects, &networkGameObjectsCount);
	App->modLinkingContext->clear();

	// Destroy all network objects
	for (uint32 i = 0; i < networkGameObjectsCount; ++i)
	{
		Destroy(networkGameObjects[i]);
	}

	App->modRender->cameraPosition = {};
}

void ModuleNetworkingClient::OnEnemyKilled()
{
	points += 20;
}
