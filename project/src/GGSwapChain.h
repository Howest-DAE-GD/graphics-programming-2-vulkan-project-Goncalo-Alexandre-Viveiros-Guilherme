#pragma once
#include <memory>
#include <vector>

#include "GGVulkan.h"
#include "GGVkHelperFunctions.h"
#include "GGImage.h"

namespace GG
{
	class Image;
}

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

namespace GG
{
	class SwapChain
	{
	public:
		SwapChain(const VkDevice& device, const VkPhysicalDevice& physicalDevice):m_Device(device),m_PhysicalDevice(physicalDevice){}
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

		VkSwapchainKHR& GetSwapChain() { return swapChain; }
		VkExtent2D& GetSwapChainExtent() { return swapChainExtent; }
		std::vector<VkImageView> GetSwapChainImageViews() { return swapChainImageViews; }
		std::vector<VkImage> GetSwapChainImages() { return swapChainImages; }
		VkFormat& GetSwapChainImgFormat() { return swapChainImageFormat; }
		VkImageLayout& GetSwapChainImgLayout() { return swapChainImageLayout; }
		VkImageView& GetDepthImageView() { return m_DepthImg->GetImageView(); }
		VkImage& GetDepthImage() { return m_DepthImg->GetImage(); }
		VkImageView& GetColorImageView() { return m_ColorImg->GetImageView(); }

	private:
		VkSwapchainKHR swapChain;
		std::vector<VkImage> swapChainImages;
		std::vector<VkImageView> swapChainImageViews;
		VkFormat swapChainImageFormat;
		VkImageLayout swapChainImageLayout;
		VkExtent2D swapChainExtent;
		std::vector <Image> m_SwapChainTotalImgs;

		std::unique_ptr<Image> m_DepthImg {std::make_unique<Image>()};
		std::unique_ptr<Image> m_ColorImg {std::make_unique<Image>()};

		VkDevice m_Device;
		VkPhysicalDevice m_PhysicalDevice;
	};
}
