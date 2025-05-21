#pragma once
#include <cstdint>
#include <optional>
#include <vector>
#include <vulkan/vulkan_core.h>

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

namespace GG
{
	class VkHelperFunctions
	{
	public:
		static uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDevice);
		static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
		static VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice);
		static VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, 
		                                    VkFormatFeatureFlags features, VkPhysicalDevice physicalDevice);
		static VkPhysicalDeviceLimits FindPhysicalDeviceLimits(VkPhysicalDevice physicalDevice);
	};
}
