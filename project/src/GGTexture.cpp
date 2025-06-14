﻿#include "GGTexture.h"

#include <stdexcept>

#include "GGVkHelperFunctions.h"
#include <cmath>

#include "GGBuffer.h"
#include "GGCommandManager.h"


using namespace GG;


Texture::Texture(std::unique_ptr<stbi_uc[]> pixels, int width, int height, VkFormat format)
	: m_TexWidth(width), m_TexHeight(height), m_ImgFormat(format), m_IsUsingPath(false)
{
	m_ManagedPixels = std::move(pixels);
	m_Pixels = m_ManagedPixels.get();
}

static uint16_t float_to_half(float f)
{
	uint32_t bits = *reinterpret_cast<uint32_t*>(&f);
	uint32_t sign = (bits >> 16) & 0x8000;
	uint32_t mantissa = bits & 0x007FFFFF;
	uint32_t exp = bits & 0x7F800000;

	if (exp >= 0x47800000) return sign | 0x7C00; // Inf/NaN
	if (exp <= 0x38000000) return sign; // Underflow/zero

	return sign | (((exp - 0x38000000) >> 13) & 0x7C00) | ((mantissa >> 13) & 0x03FF);
}

//mipmapping
void Texture::GenerateMipmaps(const CommandManager* commandManager, const VkQueue graphicsQueue, const VkDevice device, const VkPhysicalDevice physicalDevice)
{
	// Check if image format supports linear blitting
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, m_TotalImage.GetImageFormat(), &formatProperties);

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		throw std::runtime_error("texture image format does not support linear blitting!");
	}

	VkCommandBuffer commandBuffer = commandManager->BeginSingleTimeCommands(device);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = m_TotalImage.GetImage();
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = m_TexWidth;
	int32_t mipHeight = m_TexHeight;

	for (uint32_t i = 1; i < m_MipLevels; i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		const auto& image = m_TotalImage.GetImage();

		vkCmdBlitImage(commandBuffer,
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = m_MipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	commandManager->EndSingleTimeCommands(graphicsQueue, commandBuffer,device);
}

void Texture::CreateImage(Buffer* buffer, const CommandManager* commandManager, const VkQueue graphicsQueue, const VkDevice device, const VkPhysicalDevice physicalDevice)
{
	CreateTextureImage(buffer, commandManager, graphicsQueue, device, physicalDevice);
	CreateTextureImageView(device);
}
//mipmapping


//multisampling

void Texture::CreateTextureImage(Buffer* buffer, const CommandManager* commandManager, const VkQueue graphicsQueue, const VkDevice device, const VkPhysicalDevice physicalDevice)
{
	int texChannels;
	stbi_uc* loadedPixels = nullptr;
	float* loadedFloatPixels = nullptr;
	std::vector<uint16_t> halfFloatData;

	if (m_IsUsingPath) 
	{
		if (m_ImgFormat == VK_FORMAT_R16G16B16A16_SFLOAT) 
		{
			loadedFloatPixels = stbi_loadf(m_TexturePath.c_str(), &m_TexWidth, &m_TexHeight, &texChannels, STBI_rgb_alpha);
			if (!loadedFloatPixels) throw std::runtime_error("Failed to load float texture");
			size_t totalPixels = m_TexWidth * m_TexHeight * 4;
			halfFloatData.resize(totalPixels);
			for (size_t i = 0; i < totalPixels; ++i)
				halfFloatData[i] = float_to_half(loadedFloatPixels[i]);
			stbi_image_free(loadedFloatPixels);
			m_Pixels = reinterpret_cast<stbi_uc*>(halfFloatData.data());
		}

		else 
		{
			loadedPixels = stbi_load(m_TexturePath.c_str(), &m_TexWidth, &m_TexHeight, &texChannels, STBI_rgb_alpha);
			if (!loadedPixels) throw std::runtime_error("Failed to load uchar texture");
			m_Pixels = loadedPixels;
		}
	}

	uint32_t bytesPerPixel = (m_ImgFormat == VK_FORMAT_R16G16B16A16_SFLOAT) ? 8 : 4;
	VkDeviceSize imageSize = static_cast<VkDeviceSize>(m_TexWidth) * m_TexHeight * bytesPerPixel;

	m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(m_TexWidth, m_TexHeight)))) + 1;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	buffer->CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, m_Pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(device, stagingBufferMemory);

	if (loadedPixels) stbi_image_free(loadedPixels);

	m_TotalImage.CreateImage(m_TexWidth, m_TexHeight, m_MipLevels, VK_SAMPLE_COUNT_1_BIT, m_ImgFormat,
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, device, physicalDevice);

	TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, commandManager, graphicsQueue, device);
	CopyBufferToImage(stagingBuffer, commandManager, graphicsQueue, device);
	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
	GenerateMipmaps(commandManager, graphicsQueue, device, physicalDevice);
}


void Texture::TransitionImageLayout(const VkImageLayout oldLayout, const VkImageLayout newLayout, const CommandManager* commandManager, const VkQueue graphicsQueue, const VkDevice device)
{
	VkCommandBuffer commandBuffer = commandManager->BeginSingleTimeCommands(device);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = m_TotalImage.GetImage();
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = m_MipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier
	(
		commandBuffer,
		sourceStage,
		destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	commandManager->EndSingleTimeCommands(graphicsQueue, commandBuffer, device);

}

void Texture::CopyBufferToImage(const VkBuffer buffer, const CommandManager* commandManager, const VkQueue graphicsQueue, const VkDevice device) 
{
	VkCommandBuffer commandBuffer = commandManager->BeginSingleTimeCommands(device);

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { static_cast<uint32_t>(m_TexWidth), static_cast<uint32_t>(m_TexHeight), 1 };

	vkCmdCopyBufferToImage
	(
		commandBuffer,
		buffer,
		m_TotalImage.GetImage(),
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);

	commandManager->EndSingleTimeCommands(graphicsQueue,commandBuffer,device);
}

void Texture::CreateTextureImageView( const VkDevice device)
{
	m_TotalImage.CreateImageView(m_ImgFormat, VK_IMAGE_ASPECT_COLOR_BIT, m_MipLevels, device);
}

void Texture::DestroyTexture(const VkDevice device) const
{
	m_TotalImage.DestroyImg(device);
}
