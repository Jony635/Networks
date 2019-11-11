#include "Networks.h"

void ReplicationManagerServer::create(uint32 networkId)
{
	ReplicationCommand command;
	command.action = ReplicationAction::Create;
	command.networkId = networkId;

	if(map.find(networkId) == map.end())
		map[networkId] = command;
}

void ReplicationManagerServer::update(uint32 networkId)
{
	ReplicationCommand command;
	command.action = ReplicationAction::Update;
	command.networkId = networkId;

	if (map.find(networkId) == map.end())
		map[networkId] = command;
}

void ReplicationManagerServer::destroy(uint32 networkId)
{
	ReplicationCommand command;
	command.action = ReplicationAction::Destroy;
	command.networkId = networkId;

	//Destroy has preference
	map[networkId] = command;
}

void ReplicationManagerServer::write(OutputMemoryStream& packet)
{
	//Get all the stored commands into the packet and free the map memory.

	for (std::unordered_map<uint32, ReplicationCommand>::iterator it = map.begin(); it != map.end(); ++it)
	{
		ReplicationCommand& command = it->second;
		packet << command.networkId;
		packet << command.action;

		switch (command.action)
		{
			case ReplicationAction::Create:
			{
				//Get the object from LinkingContext
				GameObject* gameObject = App->modLinkingContext->getNetworkGameObject(command.networkId);

				if (gameObject)
				{
					//Serialize fields
					packet << gameObject->position.x;
					packet << gameObject->position.y;
					packet << gameObject->angle;
					packet << gameObject->color.r;
					packet << gameObject->color.g;
					packet << gameObject->color.b;
					packet << gameObject->color.a;
				}
				
				break;
			}

			case ReplicationAction::Update:
			{
				//Get the object from LinkingContext
				GameObject* gameObject = App->modLinkingContext->getNetworkGameObject(command.networkId);

				if (gameObject)
				{
					//Serialize fields
					packet << gameObject->position.x;
					packet << gameObject->position.y;
					packet << gameObject->angle;
					packet << gameObject->color.r;
					packet << gameObject->color.g;
					packet << gameObject->color.b;
					packet << gameObject->color.a;
				}

				break;
			}
		}	
	}

	map.clear();
}
