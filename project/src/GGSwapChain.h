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

		VkSwapchainKHR& GetSwapChain() { return m_SwapChain; }
		VkExtent2D& GetSwapChainExtent() { return m_SwapChainExtent; }
		std::vector<VkImageView> GetSwapChainImageViews() { return m_SwapChainImageViews; }
		std::vector<VkImage> GetSwapChainImages() { return m_SwapChainImages; }
		VkFormat& GetSwapChainImgFormat() { return m_SwapChainImageFormat; }
		VkImageLayout& GetSwapChainImgLayout() { return m_SwapChainImageLayout; }
		VkImageView& GetDepthImageView() { return m_DepthImg->GetImageView(); }
		VkImage& GetDepthImage() { return m_DepthImg->GetImage(); }
		VkImageView& GetColorImageView() { return m_ColorImg->GetImageView(); }

	private:
		VkSwapchainKHR m_SwapChain;
		std::vector<VkImage> m_SwapChainImages;
		std::vector<VkImageView> m_SwapChainImageViews;
		VkFormat m_SwapChainImageFormat;
		VkImageLayout m_SwapChainImageLayout;
		VkExtent2D m_SwapChainExtent;
		std::vector <Image> m_SwapChainTotalImgs;

		std::unique_ptr<Image> m_DepthImg {std::make_unique<Image>()};
		std::unique_ptr<Image> m_ColorImg {std::make_unique<Image>()};

		VkDevice m_Device;
		VkPhysicalDevice m_PhysicalDevice;
	};
}
