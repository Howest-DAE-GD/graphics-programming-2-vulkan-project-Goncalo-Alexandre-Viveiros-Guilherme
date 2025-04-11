#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>
namespace GG
{
	class VkErrorHandler
	{
	public:
		bool CheckValidationLayerSupport() const;

		static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
		                                          const VkAllocationCallbacks* pAllocator);

		static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator,
		                                             VkDebugUtilsMessengerEXT* pDebugMessenger);

		static VkBool32 DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

		static std::vector<const char*>& GetRequiredExtensions(std::vector<const char*>& extensions);
		static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

		const std::vector<const char*>& GetValidationLayers() const { return m_ValidationLayers; }

	private:
		const std::vector<const char*> m_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
	};
}