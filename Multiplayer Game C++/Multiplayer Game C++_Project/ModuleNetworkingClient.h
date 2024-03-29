#pragma once

#include "ModuleNetworking.h"

class ModuleNetworkingClient : public ModuleNetworking
{
	friend class ModuleNetworking;
	friend class ModuleUI;

public:

	//////////////////////////////////////////////////////////////////////
	// ModuleNetworkingClient public methods
	//////////////////////////////////////////////////////////////////////

	void setServerAddress(const char *serverAddress, uint16 serverPort);

	void setPlayerInfo(const char *playerName, uint8 spaceshipType);

	inline uint32 getPlayerNetworkID() const { return networkId; }

	void OnEnemyKilled();

private:

	//////////////////////////////////////////////////////////////////////
	// ModuleNetworking virtual methods
	//////////////////////////////////////////////////////////////////////

	bool isClient() const override { return true; }

	void onStart() override;

	void onGui() override;

	void onPacketReceived(const InputMemoryStream &packet, const sockaddr_in &fromAddress) override;

	void onUpdate() override;

	void onConnectionReset(const sockaddr_in &fromAddress) override;

	void onDisconnect() override;

	//////////////////////////////////////////////////////////////////////
	// Client state
	//////////////////////////////////////////////////////////////////////

	enum class ClientState
	{
		Stopped,
		Start,
		WaitingWelcome,
		Playing
	};

	ClientState state = ClientState::Stopped;

	std::string serverAddressStr;
	uint16 serverPort = 0;

	sockaddr_in serverAddress = {};
	std::string playerName = "player";
	uint8 spaceshipType = 0;

	uint32 playerId = 0;
	uint32 networkId = 0;

	//Game related stuff
	uint32 points = 0u;

	std::string playerBeforeName;
	uint32 playerBeforePoints = 0u;

	std::string playerAfterName;
	uint32 playerAfterPoints = 0u;

	// Input ///////////

	static const int MAX_INPUT_DATA_SIMULTANEOUS_PACKETS = 64;

	// Queue of input data
	InputPacketData inputData[MAX_INPUT_DATA_SIMULTANEOUS_PACKETS];
	uint32 inputDataFront = 0;
	uint32 inputDataBack = 0;

	float inputDeliveryIntervalSeconds = 0.05f;
	float secondsSinceLastInputDelivery = 0.0f;

	ReplicationManagerClient repManager;
	DeliveryManager delManager;

	// Timeout / ping

	double lastPacketReceivedTime = 0.0f; // NOTE(jesus): Use this to implement client timeout
	float secondsSinceLastPing = 0.0f;    // NOTE(jesus): Use this to implement ping to server
};

