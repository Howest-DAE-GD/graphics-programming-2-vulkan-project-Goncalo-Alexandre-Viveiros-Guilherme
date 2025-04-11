#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define TINYOBJLOADER_IMPLEMENTATION

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include <chrono>
#include <vector>
#include <cstdint>
#include <optional>

#include "Scene.h"

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};



struct UniformBufferObject
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
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

	void CreateInstance();
	void CreateLogicalDevice();
	void PickPhysicalDevice();
	bool IsDeviceSuitable(VkPhysicalDevice device);
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const;

	//--------------SwapChain exclusive stuff------------------------
	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device) const;
	static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats); //might have to remove static
	static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes); //might have to remove static
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;
	void CreateSwapChain();
	void RecreateSwapChain();
	void CleanupSwapChain() const;
	//--------------No more SwapChain exclusive stuff----------------

	//Image stuff??
	void CreateImageViews();


	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);

	void CreateRenderPass();

	//------------------------ Graphics Pipeline -----------------------------------
	static std::vector<char> ReadFile(const std::string& filename);
	VkShaderModule CreateShaderModule(const std::vector<char>& code);
	void CreateGraphicsPipeline();
	//------------------------ No Longer Graphics Pipeline -------------------------

	//---------------------error handling----------------------------
	bool CheckValidationLayerSupport();

	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
	                                   const VkAllocationCallbacks* pAllocator);

	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,const VkAllocationCallbacks* pAllocator,
	                                      VkDebugUtilsMessengerEXT* pDebugMessenger);

	static VkBool32 DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

	std::vector<const char*> GetRequiredExtensions();
	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	//----------------------no longer error handling----------------------------

	void CreateFramebuffers();

	//---------------------- Command stuff ------------------------------------
	void CreateCommandPool();
	void CreateCommandBuffers();
	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	//---------------------- No Command stuff ----------------------------------

	void CreateSyncObjects();

	//---------------------- Vertice -----------------------------------------
	void CreateVertexBuffer();
	void CreateIndexBuffer();
	//---------------------- No Vertice -----------------------------------------

	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
	                  VkDeviceMemory& bufferMemory);
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
	void CreateDepthResources();
	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,VkFormatFeatureFlags features) const;
	VkFormat FindDepthFormat() const;
	static bool HasStencilComponent(VkFormat format);

	//---------------------- Uniform Buffer ---------------------------------
	void CreateDescriptorSetLayout();
	void CreateUniformBuffers();
	void UpdateUniformBuffer(uint32_t currentImage) const;
	void CreateDescriptorPool();
	void CreateDescriptorSets();
	//---------------------- No Uniform Buffer ------------------------------

	//---------------------- Buffer Helper Stuff ----------------------------
	VkCommandBuffer BeginSingleTimeCommands() const;
	void EndSingleTimeCommands(VkCommandBuffer commandBuffer) const;
	//---------------------- No Buffer Helper Stuff -------------------------

	//------------------------ Texture Stuff --------------------------------
	void GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

	//multisampling
	VkSampleCountFlagBits GetMaxUsableSampleCount() const;
	void CreateColorResources();
	//multisampling

	void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples,VkFormat format,VkImageTiling tiling, 
	VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,VkDeviceMemory& imageMemory) const;

	void CreateTextureImage();
	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,uint32_t mipLevels) const;
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;
	void CreateTextureImageView();
	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) const;
	void CreateTextureSampler();
	//------------------------ No Texture Stuff -----------------------------

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
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	VkRenderPass renderPass;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;

	VkPipeline graphicsPipeline;

	Scene* m_Scene;

	//texture stuff
	uint32_t mipLevels;
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;

	VkImageView textureImageView;
	VkSampler textureSampler;

	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
	VkImage colorImage;
	VkDeviceMemory colorImageMemory;
	VkImageView colorImageView;
	//texture stuff

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	std::vector<VkImageView> swapChainImageViews;

	std::vector<VkFramebuffer> swapChainFramebuffers;
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;

	// vertice stuff
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;
	// vertice stuff

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;

	bool framebufferResized = false;

	uint32_t currentFrame = 0;

	const uint32_t WIDTH = 800;
	const uint32_t HEIGHT = 600;

	const std::string TEXTURE_PATH = "resources/textures/viking_room.png";

	const int MAX_FRAMES_IN_FLIGHT = 2;

	const std::vector<const char*> validationLayers =
	{ "VK_LAYER_KHRONOS_validation" };

	const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	VkDebugUtilsMessengerEXT debugMessenger;

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

};