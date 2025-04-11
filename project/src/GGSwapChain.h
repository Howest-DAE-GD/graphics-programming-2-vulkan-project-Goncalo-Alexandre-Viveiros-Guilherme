#pragma once
#include <vector>

#include "GGVulkan.h"
#include "GGVkHelperFunctions.h"

namespace GG
{
	class VkSwapChain
	{
	public:
		VkSwapChain(const VkDevice& device, const VkPhysicalDevice& physicalDevice):m_Device(device),m_PhysicalDevice(physicalDevice){}
		static SwapChainSupportDetails QuerySwapChainSupport(VkSurfaceKHR& surface, VkPhysicalDevice physicalDevice);
		static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats); //might have to remove static
		static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes); //might have to remove static
		static VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);

		void CreateSwapChain(VkSurfaceKHR& surface, GLFWwindow* window);
		void CreateImageViews();
		void CreateDepthResources(VkSampleCountFlagBits& msaaSamples);
		void CreateColorResources(VkSampleCountFlagBits& msaaSamples);

		void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling,
			VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) const;
			VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) const;

		void RecreateSwapChain(VkSampleCountFlagBits& msaaSamples, GLFWwindow* window, VkRenderPass& renderPass, VkSurfaceKHR& surface);
		void CleanupSwapChain() const;
		void CreateFramebuffers(VkRenderPass& renderPass);

		VkSwapchainKHR& GetSwapChain() { return swapChain; }
		VkExtent2D& GetSwapChainExtent() { return swapChainExtent; }
		std::vector<VkFramebuffer>& GetSwapChainFramebuffers() { return swapChainFramebuffers; }
		VkFormat& GetSwapChainImgFormat() { return swapChainImageFormat; }

	private:
		VkSwapchainKHR swapChain;
		std::vector<VkImage> swapChainImages;
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent;
		std::vector<VkImageView> swapChainImageViews;
		std::vector<VkFramebuffer> swapChainFramebuffers;

		VkImage depthImage;
		VkDeviceMemory depthImageMemory;
		VkImageView depthImageView;

		VkImage colorImage;
		VkDeviceMemory colorImageMemory;
		VkImageView colorImageView;
		VkDevice m_Device;
		VkPhysicalDevice m_PhysicalDevice;
	};
}
