#include <iostream>
#include <string>
#include "gl_core_4_4.h"
#include <GLFW\glfw3.h>
#include <glm\ext.hpp>

#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include "Gizmos.h"
#include "Camera.h"

#include "../../ServerApplication/GameMessages.h"

#include "BasicNetworking.h"

BasicNetworkingApplication::BasicNetworkingApplication()
{
	m_myColour.r = ((float) rand() / (RAND_MAX));
	m_myColour.g = ((float) rand() / (RAND_MAX));
	m_myColour.b = ((float) rand() / (RAND_MAX));
}

BasicNetworkingApplication::~BasicNetworkingApplication()
{}

bool BasicNetworkingApplication::startup()
{
	//Setup the basic window
	createWindow("Client Application", 1280, 720);

	Gizmos::create();

	m_pCamera = new Camera(glm::pi<float>()*0.25f, 16 / 9.f, 0.1f, 1000.0f);
	m_pCamera->setLookAtFrom(glm::vec3(10), glm::vec3(0));

	handleNetworkConnection();

	return true;
}

void BasicNetworkingApplication::shutdown()
{
	Gizmos::destroy();

	delete m_pPeerInterface;
	delete m_pCamera;
}

bool BasicNetworkingApplication::update(float deltaTime)
{
	handleNetworkMessages();

	m_pCamera->update(deltaTime);
	moveClientObject(deltaTime);

	return true;
}

void BasicNetworkingApplication::draw()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Gizmos::clear();

	for (auto it : m_gameObjects)
	{
		GameObject& obj = it;

		Gizmos::addSphere( obj.position,
			2, 32, 32, glm::vec4(obj.colour, 1));
	}

	Gizmos::draw(m_pCamera->getProjectionView());
}

void BasicNetworkingApplication::handleNetworkConnection()
{
	// Initialise the Raknet peer interface first
	m_pPeerInterface = RakNet::RakPeerInterface::GetInstance();
	initialiseClientConnection();
}

void BasicNetworkingApplication::initialiseClientConnection()
{
	// Create a socket descriptor to describe this connection
	// No data needed, as we will be connecting to a server
	RakNet::SocketDescriptor sd;

	// Now call startup - max of 1 connections (to the server)
	m_pPeerInterface->Startup(1, &sd, 1);

	std::cout << "Connecting to server at: " << IP << std::endl;

	// Now call connect to attempt to connect to the given server
	RakNet::ConnectionAttemptResult res = m_pPeerInterface->Connect(IP, PORT, nullptr, 0);

	// Finally, check to see if we connected, and if not, throw an error
	if (res != RakNet::CONNECTION_ATTEMPT_STARTED)
	{
		std::cout << "Unable to start connection, Error number: " << res << std::endl;
	}
}

void BasicNetworkingApplication::handleNetworkMessages()
{
	RakNet::Packet* packet;

	for (packet = m_pPeerInterface->Receive(); packet;
		m_pPeerInterface->DeallocatePacket(packet),
		packet = m_pPeerInterface->Receive())
	{
		switch (packet->data[0])
		{
		case ID_REMOTE_DISCONNECTION_NOTIFICATION:
			std::cout << "Another client has disconnected.\n";
			break;
		case ID_REMOTE_CONNECTION_LOST:
			std::cout << "Another client has lost the connection.\n";
			break;
		case ID_REMOTE_NEW_INCOMING_CONNECTION:
			std::cout << "Another client has connected.\n";
			break;
		case ID_CONNECTION_REQUEST_ACCEPTED:
			std::cout << "Our connection request has been accepted.\n";
			break;
		case ID_NO_FREE_INCOMING_CONNECTIONS:
			std::cout << "The server is full.\n";
			break;
		case ID_DISCONNECTION_NOTIFICATION:
			std::cout << "We have been disconnected.\n";
			break;
		case ID_CONNECTION_LOST:
			std::cout << "Connection Lost.\n";
			break;
		case ID_SERVER_CLIENT_ID:
		{
			RakNet::BitStream bsIn(packet->data, packet->length, false);
			bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
			bsIn.Read(m_uiClientId);

			std::cout << "Server has given us an id of: " << m_uiClientId << std::endl;

			createGameObject();

			break;
		}
		case ID_SERVER_FULL_OBJECT_DATA:
		{
			RakNet::BitStream bsIn(packet->data, packet->length, false);
			bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
			readObjectDataFromServer(bsIn);
			break;
		}
		default:
			std::cout << "Received a message with unkown id: " << packet->data[0] << std::endl;
			break;
		}
	}
}

void BasicNetworkingApplication::readObjectDataFromServer(RakNet::BitStream& bsIn)
{
	// Create a temp object that we will pull all the object data into
	GameObject tempGameObject;

	// Read in the information from the packet
	bsIn.Read(tempGameObject.velocity);
	bsIn.Read(tempGameObject.position);
	bsIn.Read(tempGameObject.colour);
	bsIn.Read(tempGameObject.uiOwnerClientID);
	bsIn.Read(tempGameObject.uiObjectID);
	// Check to see whether or not this object is already 
	// stored in our local object list

	bool bIsFound = false;

	for (auto& it_obj : m_gameObjects)
	{
		if (it_obj.uiObjectID == tempGameObject.uiObjectID)
		{
			bIsFound = true;

			// Update the game object
			GameObject& obj = it_obj;
			obj.velocity		= tempGameObject.velocity;
			obj.position		= tempGameObject.position;
			obj.colour			= tempGameObject.colour;
			obj.uiOwnerClientID = tempGameObject.uiOwnerClientID;
			obj.uiObjectID		= tempGameObject.uiObjectID;
		}
	}

	// If we didn't find it, then it is a new object - 
	// add it to our object list
	if (!bIsFound)
	{
		m_gameObjects.push_back(tempGameObject);
		if (tempGameObject.uiOwnerClientID == m_uiClientId)
			m_uiClientObjectIndex = m_gameObjects.size() - 1;
	}


}

void BasicNetworkingApplication::createGameObject()
{
	// Tell the server we want to create a new game
	// object that will represent us
	RakNet::BitStream bsOut;

	GameObject tempGameObject;
	tempGameObject.velocity = glm::vec3(0.0f);
	tempGameObject.position = glm::vec3(0.0f);
	tempGameObject.colour = glm::vec3(m_myColour);

	// Ensure that the write order is the same as the read order on the server!
	bsOut.Write((RakNet::MessageID)GameMessages::ID_CLIENT_CREATE_OBJECT);
	bsOut.Write(tempGameObject.velocity);
	bsOut.Write(tempGameObject.position);
	bsOut.Write(tempGameObject.colour);

	m_pPeerInterface->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED,
		0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
}

void BasicNetworkingApplication::moveClientObject(float deltaTime)
{
	// We don't have a valid client ID, so we have no game object!
	if (m_uiClientId == 0) return;

	// No game object sent to us, so we don't know who we are yet
	if (m_gameObjects.size() == 0) return;
	if (m_uiClientObjectIndex > m_gameObjects.size() - 1) return;
	bool bUpdatedObjectPosition = false;

	GameObject& myClientObject = m_gameObjects[m_uiClientObjectIndex];

	if (glfwGetKey(m_window, GLFW_KEY_UP))
	{
		myClientObject.position.z += 5 * deltaTime;
		bUpdatedObjectPosition = true;
	}
	if (glfwGetKey(m_window, GLFW_KEY_DOWN))
	{
		myClientObject.position.z -= 5 * deltaTime;
		bUpdatedObjectPosition = true;
	}

	if (bUpdatedObjectPosition)
		sendUpdatedObjectPositionToServer(myClientObject);
}

void BasicNetworkingApplication::sendUpdatedObjectPositionToServer(const GameObject& objectToUpdate)
{	
	RakNet::BitStream bsOut;

	// Ensure that the write order is the same as the read order on the server!
	bsOut.Write((RakNet::MessageID)GameMessages::ID_CLIENT_UPDATE_OBJECT_POSITION);
	bsOut.Write(objectToUpdate.uiObjectID);
	bsOut.Write(objectToUpdate.position);

	m_pPeerInterface->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED,
		0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
}