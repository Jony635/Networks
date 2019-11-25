#include "Networks.h"
#include "ReplicationManagerClient.h"

void ReplicationManagerClient::read(const InputMemoryStream& packet)
{
	while (packet.RemainingByteCount() > sizeof(uint32)) //Input last sequence number
	{
		uint32 networkId;
		packet >> networkId;

		ReplicationAction action;
		packet >> action;

		switch (action)
		{
			case ReplicationAction::Create:
			{
				vec2 position;
				float angle;
				vec4 color;
				vec2 size;
				uint8 spaceShipType;

				packet >> position.x;
				packet >> position.y;

				packet >> angle;

				packet >> color.r;
				packet >> color.g;
				packet >> color.b;
				packet >> color.a;

				packet >> size.x;
				packet >> size.y;

				packet >> spaceShipType;

				GameObject* gameObject = Instantiate();
				if (gameObject)
				{
					gameObject->position = position;
					gameObject->angle = angle;
					gameObject->color = color;
					gameObject->size = size;
		
					switch (spaceShipType)
					{
						case 0:
							gameObject->texture = App->modResources->laser;
							break;
						case 1:
							gameObject->texture = App->modResources->spacecraft1;
							break;
						case 2:
							gameObject->texture = App->modResources->spacecraft2;
							break;
						case 3:
							gameObject->texture = App->modResources->spacecraft3;
							break;
					}

					if (spaceShipType != 0)
					{
						// Create collider
						gameObject->collider = App->modCollision->addCollider(ColliderType::Player, gameObject);
						gameObject->collider->isTrigger = true;

						// Create behaviour
						gameObject->behaviour = new Spaceship;
						gameObject->behaviour->gameObject = gameObject;
					}
					else
					{
						// Create collider
						gameObject->collider = App->modCollision->addCollider(ColliderType::Laser, gameObject);
						gameObject->collider->isTrigger = true;

						// Create behaviour
						/*gameObject->behaviour = new Laser();
						gameObject->behaviour->gameObject = gameObject;*/
					}

					App->modLinkingContext->registerNetworkGameObjectWithNetworkId(gameObject, networkId);
				}

				break;
			}
			case ReplicationAction::Update:
			{
				vec2 position;
				float angle;
				vec4 color;

				packet >> position.x;
				packet >> position.y;
				packet >> angle;
				packet >> color.r;
				packet >> color.g;
				packet >> color.b;
				packet >> color.a;

				GameObject* gameObject = App->modLinkingContext->getNetworkGameObject(networkId);
				if (gameObject)
				{
					if (networkId == App->modNetClient->getPlayerNetworkID())
					{
						gameObject->position = position;
						gameObject->angle = angle;

						continue;
					}
					
					gameObject->initial_position = gameObject->position;
					gameObject->initial_angle = gameObject->angle;

					gameObject->final_position = position;
					gameObject->final_angle = angle;

					gameObject->secondsElapsed = .0f;
				}

				break;
			}
			case ReplicationAction::Destroy:
			{
				GameObject* gameObject = App->modLinkingContext->getNetworkGameObject(networkId);

				if (gameObject)
				{
					App->modLinkingContext->unregisterNetworkGameObject(gameObject);
					Destroy(gameObject);
				}
				break;
			}
		}
	}
}
