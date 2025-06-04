#include "GGGBuffer.h"

#include "GGVkDevice.h"

void GG::GBuffer::CreateImages(VkExtent2D swapChainExtent, Device* device)
{
	m_AlbedoImage.CreateImage(swapChainExtent.width, swapChainExtent.height, 1, device->GetMssaSamples(),
		VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, device->GetVulkanDevice(), device->GetVulkanPhysicalDevice());

	m_AlbedoImage.CreateImageView(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, 1, device->GetVulkanDevice());

	m_NormalMapImage.CreateImage(swapChainExtent.width, swapChainExtent.height, 1, device->GetMssaSamples(),
		VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, device->GetVulkanDevice(), device->GetVulkanPhysicalDevice());

	m_NormalMapImage.CreateImageView(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, 1, device->GetVulkanDevice());

	m_MettalicRoughnessImage.CreateImage(swapChainExtent.width, swapChainExtent.height, 1, device->GetMssaSamples(),
		VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, device->GetVulkanDevice(), device->GetVulkanPhysicalDevice());

	m_MettalicRoughnessImage.CreateImageView(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, 1, device->GetVulkanDevice());
}

GG::Image& GG::GBuffer::GetAlbedoGGImage()
{
	return m_AlbedoImage;
}

GG::Image& GG::GBuffer::GetNormalMapGGImage()
{
	return m_NormalMapImage;
}

GG::Image& GG::GBuffer::GetMettalicRoughnessGGImage()
{
	return m_MettalicRoughnessImage;
}

void GG::GBuffer::CleanUp(VkDevice device) const
{
	m_AlbedoImage.DestroyImg(device);
	m_NormalMapImage.DestroyImg(device);
	m_MettalicRoughnessImage.DestroyImg(device);
}
