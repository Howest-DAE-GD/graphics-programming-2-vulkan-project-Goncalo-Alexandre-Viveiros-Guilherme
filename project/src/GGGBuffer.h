#pragma once
#include "GGImage.h"

namespace GG
{
	class Device;
}

namespace GG
{ 
	class GBuffer
	{
	public:
		GBuffer() = default;

		void CreateImages(VkExtent2D swapChainExtent, Device* device);

		Image& GetAlbedoGGImage();
		Image& GetNormalMapGGImage();
		Image& GetMettalicRoughnessGGImage();

		void CleanUp(VkDevice device) const;
	private:
		Image m_AlbedoImage;
		Image m_NormalMapImage;
		Image m_MettalicRoughnessImage;
	};
}
