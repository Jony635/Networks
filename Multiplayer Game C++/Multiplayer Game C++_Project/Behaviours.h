#pragma once

struct Behaviour
{
	GameObject *gameObject = nullptr;

	virtual void start() { }

	virtual void update() { }

	virtual void onInput(const InputController &input) { }

	virtual void onCollisionTriggered(Collider &c1, Collider &c2) {}
};

struct Laser : public Behaviour
{
	float secondsSinceCreation = 0.0f;
	uint32 spaceShipOrigin = 0u;

	void update() override
	{
		const float pixelsPerSecond = 1000.0f;
		gameObject->position += vec2FromDegrees(gameObject->angle) * pixelsPerSecond * Time.deltaTime;

		secondsSinceCreation += Time.deltaTime;

		if (!App->modNetClient->isConnected())
			NetworkUpdate(gameObject);

		const float lifetimeSeconds = 2.0f;
		if (secondsSinceCreation > lifetimeSeconds) NetworkDestroy(gameObject);
	}
};

struct Spaceship : public Behaviour
{
	void start() override
	{
		gameObject->tag = (uint32)(Random.next() * UINT_MAX);
	}

	void onInput(const InputController &input) override
	{
		if (input.horizontalAxis != 0.0f)
		{
			const float rotateSpeed = 180.0f;
			gameObject->angle += input.horizontalAxis * rotateSpeed * Time.deltaTime;

			if(!App->modNetClient->isConnected())
				NetworkUpdate(gameObject);
		}

		if (input.actionDown == ButtonState::Pressed)
		{
			const float advanceSpeed = 200.0f;
			gameObject->position += vec2FromDegrees(gameObject->angle) * advanceSpeed * Time.deltaTime;
			if (!App->modNetClient->isConnected())
				NetworkUpdate(gameObject);
		}

		if (input.actionLeft == ButtonState::Press && !App->modNetClient->isConnected())
		{
			//Only spawn bullets if it is the server who is processing inputs.
			GameObject* laser = App->modNetServer->spawnBullet(gameObject);
			laser->tag = gameObject->tag;

			Laser* laserBehavior = (Laser*)laser->behaviour;
			laserBehavior->spaceShipOrigin = gameObject->networkId;
		}
	}

	void onCollisionTriggered(Collider &c1, Collider &c2) override
	{
		if (!App->modNetClient->isConnected() && c2.type == ColliderType::Laser && c2.gameObject->tag != gameObject->tag)
		{
			NetworkDestroy(c2.gameObject); // Destroy the laser

			// NOTE(jesus): spaceship was collided by a laser
			// Be careful, if you do NetworkDestroy(gameObject) directly,
			// the client proxy will poing to an invalid gameObject...
			// instead, make the gameObject invisible or disconnect the client.

			App->modNetServer->SpaceShipDestroy(c1.gameObject, ((Laser*)c2.gameObject->behaviour)->spaceShipOrigin);
		}
	}
};