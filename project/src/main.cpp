#include <iostream>
#include "GGVulkan.h"
#include "Scene.h"

int main()
{
	GGVulkan app;

	try
	{
		Scene* newScene = new Scene();
		newScene->AddFileToScene("resources/models/viking_room.obj");
		newScene->BindTextureToMesh("resources/models/viking_room.obj", "resources/textures/viking_room.png");
		newScene->AddFileToScene("resources/models/tralalero tralala.fbx");
		newScene->BindTextureToMesh("resources/models/tralalero tralala.fbx", "resources/textures/shaded.png");
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
