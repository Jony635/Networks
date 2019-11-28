#include "Networks.h"
#include "ModuleGameObject.h"

void GameObject::releaseComponents()
{
	if (behaviour != nullptr)
	{
		delete behaviour;
		behaviour = nullptr;
	}
	if (collider != nullptr)
	{
		App->modCollision->removeCollider(collider);
		collider = nullptr;
	}
}

bool ModuleGameObject::init()
{
	return true;
}

bool ModuleGameObject::preUpdate()
{
	for (GameObject &gameObject : gameObjects)
	{
		if (gameObject.state == GameObject::NON_EXISTING) continue;

		if (gameObject.state == GameObject::DESTROYING)
		{
			gameObject.releaseComponents();
			gameObject = GameObject();
			gameObject.state = GameObject::NON_EXISTING;
		}
		else if (gameObject.state == GameObject::CREATING)
		{
			if (gameObject.behaviour != nullptr && gameObject.enabled)
				gameObject.behaviour->start();
			gameObject.state = GameObject::UPDATING;
		}

		if (App->modNetClient->isConnected())
		{
			if (gameObject.networkId != 0 && gameObject.networkId != App->modNetClient->getPlayerNetworkID())
			{
				//External networked GameObject.
				//INTERPOLATION.

				if (gameObject.secondsElapsed != -1.0f)
				{
					if (gameObject.secondsElapsed > 0.2f)
					{
						gameObject.secondsElapsed = -1.0f;
						continue;
					}

					gameObject.position = Interpolate(gameObject.initial_position, gameObject.final_position, gameObject.secondsElapsed);
					gameObject.angle = Interpolate(gameObject.initial_angle, gameObject.final_angle, gameObject.secondsElapsed);

					gameObject.secondsElapsed += Time.deltaTime;
				}	
			}
		}		
	}

	return true;
}

bool ModuleGameObject::update()
{
	for (GameObject &gameObject : gameObjects)
	{
		if (gameObject.state == GameObject::UPDATING)
		{
			if (gameObject.behaviour != nullptr && gameObject.enabled)
				gameObject.behaviour->update();
		}
	}

	return true;
}

bool ModuleGameObject::postUpdate()
{
	return true;
}

bool ModuleGameObject::cleanUp()
{
	for (GameObject &gameObject : gameObjects)
	{
		gameObject.releaseComponents();
	}

	return true;
}

GameObject * ModuleGameObject::Instantiate()
{
	for (GameObject &gameObject : App->modGameObject->gameObjects)
	{
		if (gameObject.state == GameObject::NON_EXISTING)
		{
			gameObject.state = GameObject::CREATING;
			return &gameObject;
		}
	}

	ASSERT(0); // NOTE(jesus): You need to increase MAX_GAME_OBJECTS in case this assert crashes
	return nullptr;
}

void ModuleGameObject::Destroy(GameObject * gameObject)
{
	ASSERT(gameObject->networkId == 0); // NOTE(jesus): If it has a network identity, it must be destroyed by the Networking module first
	
	gameObject->state = GameObject::DESTROYING;
}

vec2 ModuleGameObject::Interpolate(vec2& initial, vec2& final, float timeElapsed)
{
	if (timeElapsed > 0.2f)
		return final;

	vec2 diff = final - initial;
	return (diff * timeElapsed / 0.2f) + initial;
}

float ModuleGameObject::Interpolate(float initial, float final, float timeElapsed)
{
	if (timeElapsed > 0.2f)
		return final;

	float diff = final - initial;
	return (diff * timeElapsed / 0.2f) + initial;
}

GameObject * Instantiate()
{
	GameObject *result = ModuleGameObject::Instantiate();
	return result;
}

void Destroy(GameObject * gameObject)
{
	ModuleGameObject::Destroy(gameObject);
}
