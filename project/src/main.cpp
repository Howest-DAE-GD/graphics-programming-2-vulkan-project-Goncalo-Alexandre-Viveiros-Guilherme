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
		newScene->AddLight(Light{ {7,1,0},10, {60,200,30} });
		newScene->AddLight(Light{ {1.5,0,0},5, {200,60,30} });
		newScene->AddLight(Light{ {1.5,0,1},5 ,{30,60,200}});

		//newScene->AddFileToScene("resources/models/viking_room.obj");
		//newScene->BindTextureToMesh("resources/models/viking_room.obj", "resources/textures/viking_room.png", VK_FORMAT_B8G8R8A8_SRGB);
		//newScene->AddFileToScene("resources/models/tralalero_tralala.glb");
		//newScene->AddFileToScene("resources/models/porsche.glb");
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
