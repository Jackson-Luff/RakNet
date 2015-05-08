#pragma once
#include <vector>
#include <glm\glm.hpp>

#include "BitStream.h"
#include "BaseApplication.h"
#include "../../ServerApplication/GameObject.h"

class Camera;

namespace RakNet
{
	class RakPeerInterface;
}

class BasicNetworkingApplication : public BaseApplication
{
public:
	BasicNetworkingApplication();
	~BasicNetworkingApplication();

	virtual bool startup();
	virtual void shutdown();
	virtual bool update(float deltaTime);
	virtual void draw();

	// Initialise the connection
	void handleNetworkConnection();
	void initialiseClientConnection();

	// Handle incoming packets
	void handleNetworkMessages();

	// Read in object data from specific packets
	void readObjectDataFromServer(RakNet::BitStream& bsIn);

	// Creates and sends data off to sever
	void createGameObject();

	// Makes object moveable
	void moveClientObject(float deltaTime);

	// Sends updated object to all Clients / Server
	void sendUpdatedObjectPositionToServer(const GameObject& objectToUpdate);

private:
	
	// Server stuff

	RakNet::RakPeerInterface* m_pPeerInterface;

	const char* IP = "127.0.0.1";
	const unsigned short PORT = 5456;
	unsigned int m_uiClientId;
	unsigned int m_uiClientObjectIndex;

	// Render stuff

	glm::vec4 m_myColour;
	std::vector<GameObject> m_gameObjects;

	Camera* m_pCamera;
};