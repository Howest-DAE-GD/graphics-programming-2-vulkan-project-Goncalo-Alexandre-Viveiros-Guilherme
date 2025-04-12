#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>

namespace GG
{
	class DescriptorManager
	{
	public:
		void CreateDescriptorPool(VkDevice device, int maxFramesInFlight);
		void CreateDescriptorSets(VkImageView imageView, VkSampler sampler, int maxFramesInFlight, VkDevice device, std::vector<VkBuffer> uniformBuffers);
		void CreateDescriptorSetLayout(VkDevice device);

		VkDescriptorSetLayout& GetDescriptorSetLayout() { return m_DescriptorSetLayout; }
		std::vector<VkDescriptorSet>& GetDescriptorSets() { return m_DescriptorSets; }
		VkDescriptorPool& GetDescriptorPool() { return m_DescriptorPool; }
	private:
		VkDescriptorPool m_DescriptorPool = nullptr;
		std::vector<VkDescriptorSet> m_DescriptorSets;
		VkDescriptorSetLayout m_DescriptorSetLayout = nullptr;
	};
}
