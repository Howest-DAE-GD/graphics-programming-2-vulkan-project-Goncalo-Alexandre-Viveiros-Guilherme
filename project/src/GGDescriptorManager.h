#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>

namespace GG
{
	class DescriptorManager
	{
	public:
		void CreateDescriptorPool(VkDevice device, int maxFramesInFlight, int textureCounts);
		void CreateDescriptorSets(std::vector<VkImageView> imageViews, VkSampler sampler, int maxFramesInFlight, VkDevice device, std::vector<VkBuffer> uniformBuffers);
		void CreateDescriptorSetLayout(VkDevice device, int textureCounts);

		VkDescriptorSetLayout& GetDescriptorSetLayout() { return m_DescriptorSetLayout; }
		std::vector<VkDescriptorSet>& GetDescriptorSets() { return m_DescriptorSets; }
		VkDescriptorPool& GetDescriptorPool() { return m_DescriptorPool; }

		void Destroy(VkDevice device) const;
	private:
		VkDescriptorPool m_DescriptorPool = nullptr;
		std::vector<VkDescriptorSet> m_DescriptorSets;
		VkDescriptorSetLayout m_DescriptorSetLayout = nullptr;
	};
}
