#define STB_IMAGE_IMPLEMENTATION
#include <iostream>
#include "GGVulkan.h"
#include "Scene.h"
#include "Time.h"

int main()
{
	GGVulkan app;

	try
	{
		Scene* newScene = new Scene();

		newScene = new Scene();
		newScene->AddFileToScene("resources/models/Sponza/Sponza.gltf");
		app.AddScene(newScene);

		//newScene->AddFileToScene("resources/models/viking_room.obj");
		//newScene->BindTextureToMesh("resources/models/viking_room.obj", "resources/textures/viking_room.png");
		//newScene->AddFileToScene("resources/models/tralalero_tralala.glb");
		//app.AddScene(newScene);

		app.Run();
	}

	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
