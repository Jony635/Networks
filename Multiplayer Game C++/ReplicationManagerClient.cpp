#include "Networks.h"
#include "ReplicationManagerClient.h"

void ReplicationManagerClient::read(const InputMemoryStream& packet)
{
	while (packet.RemainingByteCount() > 0)
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

				packet >> position.x;
				packet >> position.y;
				packet >> angle;
				packet >> color.r;
				packet >> color.g;
				packet >> color.b;
				packet >> color.a;

				GameObject* gameObject = Instantiate();
				if (gameObject)
				{
					gameObject->position = position;
					gameObject->angle = angle;
					gameObject->color = color;

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
					gameObject->position = position;
					gameObject->angle = angle;
					gameObject->color = color;
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