#pragma once
#include <memory>
#include <vulkan/vulkan_core.h>

#include "GGDescriptorManager.h"
#include "GGPipeLine.h"

namespace GG
{
	class Device;
	class Image;

	class BlitPass
	{
	public:
		BlitPass();
		~BlitPass() = default;

		void CreateImage(VkExtent2D swapChainExtent, Device* device);
		void CreateDescriptorSets(Device* device, DescriptorManager* descriptorManager, int maxFramesInFlight);
		static void CreateDescriptorPool(Device* device, DescriptorManager* descriptorManager,int maxFramesInFlight);
		static void CreateDescriptorSetLayout(Device* device, DescriptorManager* descriptorManager);
		void CreateBlitPipeline(Device* device, DescriptorManager* descriptorManager, VkFormat swapchainFormat) const;

		Pipeline* GetPipeline() const { return m_Pipeline; }
		Image* GetImage() const { return m_Image; }

		void Cleanup(Device* device) const;
	private:
		Image* m_Image;
		Pipeline* m_Pipeline = nullptr;
	};
}
