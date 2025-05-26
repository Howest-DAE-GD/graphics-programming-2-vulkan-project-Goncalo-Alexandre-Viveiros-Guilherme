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
	void CreateSyncObjects();

	void CreateDescriptorSets() const;
	void CreateDescriptorSets4PrePass() const;

	void CreateDescriptorPool() const;
	void CreateDescriptorPool4PrePass() const;

	void CreateDescriptorSetLayout() const;
	void CreateDescriptorSetLayout4PrePass() const;

	void CreateGraphicsPipeline() const;
	void CreateDepthPrePassPipeline() const;
	void CreateGBufferPipeline();

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
	std::vector<Scene*> m_Scenes;
	Scene* m_CurrentScene									= nullptr;
	GG::SwapChain* m_VkSwapChain							= nullptr;
	GG::Buffer* m_pBuffer									= nullptr;
	GG::DescriptorManager* m_pDescriptorManager				= nullptr;
	GG::CommandManager* m_pCommandManager					= nullptr;
	GG::Pipeline* m_pPipeline								= nullptr;
	GG::Pipeline* m_pPrePassPipeline						= nullptr;
	GG::VkErrorHandler m_ErrorHandler;
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