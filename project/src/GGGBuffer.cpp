#include "GGGBuffer.h"

#include "GGVkDevice.h"

void GG::GBuffer::CreateImages(VkExtent2D swapChainExtent, Device* device)
{
	m_AlbedoImage.CreateImage(swapChainExtent.width, swapChainExtent.height, 1, device->GetMssaSamples(),
		VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, device->GetVulkanDevice(), device->GetVulkanPhysicalDevice());

	m_AlbedoImage.CreateImageView(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, 1, device->GetVulkanDevice());
}

VkImageView GG::GBuffer::GetAlbedoImageView()
{
	return m_AlbedoImage.GetImageView();
}

VkImage GG::GBuffer::GetAlbedoImage()
{
	return m_AlbedoImage.GetImage();
}

GG::Image& GG::GBuffer::GetAlbedoGGImage()
{
	return m_AlbedoImage;
}

void GG::GBuffer::CleanUp(VkDevice device) const
{
	m_AlbedoImage.DestroyImg(device);
	m_NormalMapImage.DestroyImg(device);
	m_MettalicRoughnessImage.DestroyImg(device);
}
