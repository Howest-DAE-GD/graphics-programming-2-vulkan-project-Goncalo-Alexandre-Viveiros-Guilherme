#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <vector>
#include <cstdint>

#include "GGCamera.h"
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

	static bool HasStencilComponent(VkFormat format);

	void Cleanup() const;

	//-------------Non Tutorial Functions-------------------
	void AddScene(Scene* sceneToAdd);

private:
	VkInstance m_Instance									= nullptr;
	GG::Device* m_Device									= nullptr;
	GLFWwindow* m_Window									= nullptr;

	VkSurfaceKHR m_Surface									= nullptr;

	VkRenderPass m_RenderPass								= nullptr;

	//After reformat stuff
	Scene* m_Scene											= nullptr;
	GG::SwapChain* m_VkSwapChain							= nullptr;
	GG::Buffer* m_pBuffer									= nullptr;
	GG::DescriptorManager* m_pDescriptorManager				= nullptr;
	GG::CommandManager* m_pCommandManager					= nullptr;
	GG::Pipeline* m_pPipeline								= nullptr;
	GG::VkErrorHandler m_ErrorHandler;
	GG::Camera m_Camera;
	//////////////////////////
	
	std::vector<VkSemaphore> m_ImageAvailableSemaphores;
	std::vector<VkSemaphore> m_RenderFinishedSemaphores;
	std::vector<VkFence> m_InFlightFences;

	bool m_FramebufferResized								= false;


	const int m_MaxFramesInFlight							= 2;
	uint32_t m_CurrentFrame									= 0;

	const uint32_t m_Width									= 800;
	const uint32_t m_Height									= 600;

	VkDebugUtilsMessengerEXT m_DebugMessenger				= nullptr;

#ifdef NDEBUG
	const bool m_EnableValidationLayers						= false;
#else
	const bool m_EnableValidationLayers						= true;
#endif

};