#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <vector>
#include <cstdint>

#include "GGCommandManager.h"
#include "VkErrorHandler.h"

namespace GG
{
	class Device;
}

namespace GG
{
	class DescriptorManager;
	class Texture;
	class Pipeline;
	class Buffer;
	class Image;
	class SwapChain;
}

//forward declarations
class GLFWwindow;



class GGVulkan
{
public:
	void Run();

	void InitWindow();

	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);

	void InitVulkan();

	void CreateSurface();

	void MainLoop();

	void DrawFrame();

	void SetupDebugMessenger();

	std::vector<const char*> GetRequiredExtensions() const;
	void CreateInstance();
	void CreateRenderPass();
	void CreateSyncObjects();

	//---------------------- Vertice -----------------------------------------
	void CreateVertexBuffer() const;
	void CreateIndexBuffer() const;
	//---------------------- No Vertice -----------------------------------------

	static bool HasStencilComponent(VkFormat format);

	void Cleanup() const;

	//-------------Non Tutorial Functions-------------------
	void AddScene(Scene* sceneToAdd);

private:
	VkInstance instance;
	GG::Device* m_Device;
	GLFWwindow* window;

	VkSurfaceKHR surface;

	VkRenderPass renderPass;

	//After reformat stuff
	Scene* m_Scene;
	GG::SwapChain* m_VkSwapChain;
	GG::Buffer* m_pBuffer;
	GG::DescriptorManager* m_pDescriptorManager;
	GG::CommandManager* m_pCommandManager;
	GG::Texture* m_pTexture;
	GG::Pipeline* m_pPipeline;
	GG::VkErrorHandler m_ErrorHandler;
	//////////////////////////
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;

	bool framebufferResized = false;


	const int MAX_FRAMES_IN_FLIGHT{ 2 };
	uint32_t currentFrame = 0;

	const uint32_t WIDTH = 800;
	const uint32_t HEIGHT = 600;

	VkDebugUtilsMessengerEXT debugMessenger;

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

};