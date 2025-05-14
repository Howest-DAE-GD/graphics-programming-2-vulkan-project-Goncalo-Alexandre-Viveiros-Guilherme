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
		void GenerateMipmaps(const CommandManager* commandManager, VkQueue graphicsQueue, VkDevice device, VkPhysicalDevice physicalDevice);


		//multisampling
		void CreateImage(Buffer* buffer, const CommandManager* commandManager, VkQueue graphicsQueue, VkDevice device, VkPhysicalDevice physicalDevice);
		void CopyBufferToImage(VkBuffer buffer, const CommandManager* commandManager, VkQueue graphicsQueue, VkDevice device);
		void CreateTextureImageView(VkDevice device);

		VkImageView& GetImageView() { return m_TotalImage.GetImageView(); }
		VkImage& GetImage() { return m_TotalImage.GetImage(); }
		VkFormat& GetImageFormat() { return m_TotalImage.GetImageFormat(); }

		void DestroyTexture(VkDevice device) const;
	private:
		void CreateTextureImage(Buffer* buffer, const CommandManager* commandManager, VkQueue graphicsQueue, VkDevice device, VkPhysicalDevice physicalDevice);
		void TransitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout, const CommandManager* commandManager, VkQueue graphicsQueue, VkDevice device);

		uint32_t m_MipLevels;
		Image m_TotalImage;

		const std::string m_TexturePath;

		int32_t m_TexWidth;
		int32_t m_TexHeight;
	};
}
