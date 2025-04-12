#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define TINYOBJLOADER_IMPLEMENTATION



#include <chrono>
#include <vector>
#include <cstdint>


#include "GGCommandManager.h"
#include "Scene.h"
#include "VkErrorHandler.h"

namespace GG
{
	class Texture;
}

namespace GG
{
	class DescriptorManager;
}

namespace GG
{
	class Buffer;
}

namespace GG
{
	class Image;
}

namespace GG
{
	class SwapChain;
}

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

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
	void CreateLogicalDevice();
	void PickPhysicalDevice();
	bool IsDeviceSuitable(VkPhysicalDevice physicalDevice);
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const;

	void CreateRenderPass();

	//------------------------ Graphics Pipeline -----------------------------------
	static std::vector<char> ReadFile(const std::string& filename);
	VkShaderModule CreateShaderModule(const std::vector<char>& code);
	void CreateGraphicsPipeline();
	//------------------------ No Longer Graphics Pipeline -------------------------

	void CreateSyncObjects();

	//---------------------- Vertice -----------------------------------------
	void CreateVertexBuffer();
	void CreateIndexBuffer();
	//---------------------- No Vertice -----------------------------------------

	static bool HasStencilComponent(VkFormat format);

	void Cleanup();

	//-------------Non Tutorial Functions-------------------
	void AddScene(Scene* sceneToAdd);

private:
	VkInstance instance;
	VkDevice device;
	VkQueue graphicsQueue;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	GLFWwindow* window;

	VkSurfaceKHR surface;
	VkQueue presentQueue;

	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;

	VkPipeline graphicsPipeline;

	//After reformat stuff
	Scene* m_Scene;
	GG::SwapChain* m_VkSwapChain;
	GG::Buffer* m_pBuffer;
	GG::DescriptorManager* m_pDescriptorManager;
	GG::CommandManager* m_pCommandManager;
	GG::Texture* m_pTexture;

	//////////////////////////
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;

	bool framebufferResized = false;


	const int MAX_FRAMES_IN_FLIGHT{ 2 };
	uint32_t currentFrame = 0;

	const uint32_t WIDTH = 800;
	const uint32_t HEIGHT = 600;


	const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	VkDebugUtilsMessengerEXT debugMessenger;

	GG::VkErrorHandler m_ErrorHandler;

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

};