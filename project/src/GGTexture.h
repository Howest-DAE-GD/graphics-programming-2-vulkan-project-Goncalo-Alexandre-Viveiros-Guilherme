#pragma once
#include <string>
#include "GGImage.h"



namespace GG
{
	class CommandManager;
	class Buffer;

	class Texture
	{
	public:
		Texture(const std::string& texturePath) : m_TexturePath(texturePath){}
		void GenerateMipmaps(CommandManager* commandManager, VkQueue graphicsQueue, VkDevice device, VkPhysicalDevice physicalDevice);

		//multisampling
		void GetMaxUsableSampleCount(VkPhysicalDevice physicalDevice);

		//multisampling
		void CreateTextureImage(Buffer* buffer, CommandManager* commandManager, VkQueue graphicsQueue, VkDevice device, VkPhysicalDevice physicalDevice);
		void TransitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout, CommandManager* commandManager, VkQueue graphicsQueue, VkDevice device);
		void CopyBufferToImage(VkBuffer buffer, CommandManager* commandManager, VkQueue graphicsQueue, VkDevice device);
		void CreateTextureImageView(VkDevice device);
		void CreateTextureSampler(VkDevice device, VkPhysicalDevice physicalDevice);

		VkSampleCountFlagBits& GetMssaSamples() { return m_MsaaSamples; }
		VkSampler& GetTextureSampler() { return m_TextureSampler; }

		VkImageView& GetImageView() { return m_TotalImage.GetImageView(); }

		void DestroyTexture(VkDevice device);
	private:
		uint32_t m_MipLevels;

		VkSampler m_TextureSampler;

		VkSampleCountFlagBits m_MsaaSamples = VK_SAMPLE_COUNT_1_BIT;

		Image m_TotalImage;

		const std::string m_TexturePath;

		int32_t m_TexWidth;
		int32_t m_TexHeight;
	};
}
