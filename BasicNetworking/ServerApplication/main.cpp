#include <iostream>

#include "Server.h"

int main()
{
	Server* server = new Server();
	server->run();
	delete server;

	return 0;
}