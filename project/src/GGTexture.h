#pragma once

#include <string>
#include "GGImage.h"
#include <stb_image.h>

namespace GG
{
	class CommandManager;
	class Buffer;

	class Texture
	{
	public:
		Texture(const std::string& texturePath) : m_TexturePath(texturePath), m_IsUsingPath(true){}
		Texture(stbi_uc* pixels, int32_t texWidth, int32_t texHeight) :
		m_IsUsingPath(false),m_Pixels(pixels),m_TexWidth(texWidth),m_TexHeight(texHeight){}

		void GenerateMipmaps(const CommandManager* commandManager, VkQueue graphicsQueue, VkDevice device, VkPhysicalDevice physicalDevice);


		//multisampling
		void CreateImage(Buffer* buffer, const CommandManager* commandManager, VkQueue graphicsQueue, VkDevice device, VkPhysicalDevice physicalDevice);
		void CopyBufferToImage(VkBuffer buffer, const CommandManager* commandManager, VkQueue graphicsQueue, VkDevice device);
		void CreateTextureImageView(VkDevice device);

		VkImageView& GetImageView() { return m_TotalImage.GetImageView(); }
		Image& GetGGImage() { return m_TotalImage; }
		VkImage& GetImage() { return m_TotalImage.GetImage(); }
		VkFormat& GetImageFormat() { return m_TotalImage.GetImageFormat(); }
		VkImageLayout& GetImageLayout() { return m_TotalImage.GetCurrentLayout(); }

		void DestroyTexture(VkDevice device) const;
	private:
		void CreateTextureImage(Buffer* buffer, const CommandManager* commandManager, VkQueue graphicsQueue, VkDevice device, VkPhysicalDevice physicalDevice);
		void TransitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout, const CommandManager* commandManager, VkQueue graphicsQueue, VkDevice device);

		uint32_t m_MipLevels;
		Image m_TotalImage;

		const std::string m_TexturePath;
		stbi_uc* m_Pixels;

		bool m_IsUsingPath;
		int32_t m_TexWidth;
		int32_t m_TexHeight;
		int m_DesiredChannels;
	};
}
