#pragma once
#include <glm\glm.hpp>

enum SyncType
{
	POSITION_ONLY,
	LERP,
	INTERPOLATION
};

struct GameObject
{
	unsigned int uiOwnerClientID;
	unsigned int uiObjectID;

	glm::vec3 colour;
	glm::vec3 position;
	glm::vec3 velocity;

	SyncType eSyncType;
};