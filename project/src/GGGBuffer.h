#pragma once
#include "GGImage.h"
#include "GGPipeLine.h"

class Scene;

namespace GG
{
	class Buffer;
	class DescriptorManager;
	class Device;

	class GBuffer
	{
	public:
		GBuffer();

		void CreateImages(VkExtent2D swapChainExtent, Device* device);

		Image& GetAlbedoGGImage();
		Image& GetNormalMapGGImage();
		Image& GetMettalicRoughnessGGImage();

		void CreatePipeline(Device* device, DescriptorManager* descriptorManager);
		void CreateDescriptorSets(Scene* currentScene, Device* device, DescriptorManager* descriptorManager, Buffer* buffer, int maxFramesInFlight);
		void CreateDescriptorSetLayout(Device* device, DescriptorManager* descriptorManager);
		void CreateDescriptorPool(Device* device, DescriptorManager* descriptorManager, int maxFramesInFlight);

		Pipeline* GetPipeline() const {return m_Pipeline;}

		void CleanUp(VkDevice device) const;
	private:
		GG::Pipeline* m_Pipeline = nullptr;
		Image m_AlbedoImage;
		Image m_NormalMapImage;
		Image m_MettalicRoughnessImage;
	};
}
