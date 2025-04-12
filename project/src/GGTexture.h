#pragma once
#include <string>
#include "GGImage.h"



namespace GG
{
	class Texture
	{
	public:
		void GenerateMipmaps();

		//multisampling
		VkSampleCountFlagBits GetMaxUsableSampleCount() const;

		//multisampling
		void CreateTextureImage();
		void TransitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
		void CopyBufferToImage(VkBuffer buffer);
		void CreateTextureImageView();
		void CreateTextureSampler();
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
