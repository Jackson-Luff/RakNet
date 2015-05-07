#include <iostream>
#include <string>
#include <chrono>
#include <thread>

#include "GameMessages.h"
#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"

void handleNetworkMessages(RakNet::RakPeerInterface* a_pPeerInterface)
{
	RakNet::Packet* packet = nullptr;

	while (true)
	{
		for (packet = a_pPeerInterface->Receive(); packet;
			a_pPeerInterface->DeallocatePacket(packet),
			packet = a_pPeerInterface->Receive())
		{
			switch (packet->data[0])
			{
			case ID_NEW_INCOMING_CONNECTION:
				std::cout << "A connection is incoming.\n";
				break;
			case ID_DISCONNECTION_NOTIFICATION:
				std::cout << "A connection has disconnected.\n";
				break;
			case ID_CONNECTION_LOST:
				std::cout << "A client lost the connection.\n";
				break;
			case ID_SERVER_TEXT_MESSAGE:
			{
				RakNet::BitStream bsIn(packet->data, packet->length, false);
				bsIn.IgnoreBytes(sizeof(RakNet::MessageID));

				RakNet::RakString str;
				bsIn.Read(str);
				std::cout << str.C_String() << std::endl;
				break;
			}
			default:
				std::cout << "Received a message with unkown id" << packet->data[0]; 
				break;
			}
		}
	}
}

void sendClientPing(RakNet::RakPeerInterface* a_pPeerInterface)
{
	while (true)
	{
		RakNet::BitStream bs;
		bs.Write((RakNet::MessageID)GameMessages::ID_SERVER_TEXT_MESSAGE);
		bs.Write("Ping!");

		a_pPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0,
			RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);

		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

int main()
{
	const unsigned short PORT = 5456;
	RakNet::RakPeerInterface* pPeerInterface = nullptr;

	// Startup the server, and start it listening to clients
	std::cout << "Starting up the server..." << std::endl;

	// Initialise the Raknet peer interface first
	pPeerInterface = RakNet::RakPeerInterface::GetInstance();

	// Create a socket descriptor to describe this connection
	RakNet::SocketDescriptor sd(PORT, 0);

	// Now call startup - max of 32 connections, on the assigned port
	pPeerInterface->Startup(32, &sd, 1);
	pPeerInterface->SetMaximumIncomingConnections(32);

	handleNetworkMessages(pPeerInterface);

	std::thread pingThread(sendClientPing, pPeerInterface);

	return 0;
}