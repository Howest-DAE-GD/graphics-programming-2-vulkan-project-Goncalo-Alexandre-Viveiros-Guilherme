#include <iostream>
#include "GGVulkan.h"

int main()
{
	GGVulkan app;

	try
	{
		Scene* newScene = new Scene();
		newScene->AddModel(Model("resources/models/viking_room.obj", "resources/textures/viking_room.png"));
		app.AddScene(newScene);
		app.Run();
	}

	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
