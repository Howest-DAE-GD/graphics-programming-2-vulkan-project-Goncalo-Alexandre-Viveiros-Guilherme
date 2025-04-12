#pragma once
#include <vulkan/vulkan_core.h>

namespace GG
{
	class Image
	{
	public:
		void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling,
			VkImageUsageFlags usage, VkMemoryPropertyFlags properties, const VkDevice& device, const VkPhysicalDevice& physicalDevice);

		void CreateImageView(VkFormat format,VkImageAspectFlags aspectFlags,uint32_t mipLevels, const VkDevice& device);

		VkImageView& GetImageView() { return m_ImageView; }
		VkImage& GetImage() { return m_Image; }
		VkFormat& GetImageFormat() { return m_Format; }

		void DestroyImg(const VkDevice& device) const;

	private:
		VkImage m_Image;
		VkDeviceMemory m_ImageMemory;
		VkImageView m_ImageView;
		VkFormat m_Format;
	};
}
