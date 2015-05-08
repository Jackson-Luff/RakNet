#include <time.h>
#include "BasicNetworking.h"

int main() {
	
	srand((unsigned int)time(NULL));

	BaseApplication* app = new BasicNetworkingApplication();
	if (app->startup())
		app->run();
	app->shutdown();

	return 0;
}