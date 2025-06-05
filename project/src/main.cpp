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
		newScene->AddLight(PointLight{ {1.5,1,-1},30,{255,0,0}});
		newScene->AddLight(PointLight{ {7,1,0},   30,{0,255,0}});
		newScene->AddLight(PointLight{ {1.5,1,1}, 30,{0,0,255}});
		newScene->AddLight(DirectionalLight{ {0,0,-1}, 500 ,{209,98,14}});

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
