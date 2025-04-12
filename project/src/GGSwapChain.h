#pragma once
#include <vector>

#include "GGVulkan.h"
#include "GGVkHelperFunctions.h"
#include "GGVkImage.h"

namespace GG
{
	class VkTotalImage;
}

namespace GG
{
	class VkSwapChain
	{
	public:
		VkSwapChain(const VkDevice& device, const VkPhysicalDevice& physicalDevice):m_Device(device),m_PhysicalDevice(physicalDevice){}
		static SwapChainSupportDetails QuerySwapChainSupport(VkSurfaceKHR& surface, VkPhysicalDevice physicalDevice);
		static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats); 
		static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		static VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);

		void CreateSwapChain(VkSurfaceKHR& surface, GLFWwindow* window);
		void CreateImageViews();
			VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) const;
		void CreateDepthResources(const VkSampleCountFlagBits& msaaSamples) const;
		void CreateColorResources(const VkSampleCountFlagBits& msaaSamples) const;

		void RecreateSwapChain(const VkSampleCountFlagBits& msaaSamples, GLFWwindow* window, VkRenderPass& renderPass, VkSurfaceKHR& surface);
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
		std::vector <VkTotalImage> m_SwapChainTotalImgs;

		std::unique_ptr<VkTotalImage> m_DepthImg {std::make_unique<VkTotalImage>()};
		std::unique_ptr<VkTotalImage> m_ColorImg {std::make_unique<VkTotalImage>()};

		VkDevice m_Device;
		VkPhysicalDevice m_PhysicalDevice;
	};
}
