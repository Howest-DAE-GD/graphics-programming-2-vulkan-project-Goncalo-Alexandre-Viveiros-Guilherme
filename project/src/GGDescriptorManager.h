#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>

struct DescriptorSetLayoutContext
{
	std::vector<VkDescriptorSetLayoutBinding> LayoutBindings;
	std::vector<VkDescriptorBindingFlags> BindingFlags;
	int DescriptorSetLayoutIndex;

	void AddDescriptorSetLayout(VkDescriptorSetLayoutBinding layoutBinding)
	{
		LayoutBindings.emplace_back(layoutBinding);
	}
};
	
struct DescriptorSetsContext
{
	VkDescriptorSetLayout DescriptorSetLayout;
	std::vector<VkWriteDescriptorSet> DescriptorSetWrites;
	std::vector <std::vector<VkWriteDescriptorSet>> FrameDescriptorSetWrites;
	VkDescriptorSetAllocateInfo AllocateInfo;
	uint32_t VariableCount{1};

	std::vector<VkDescriptorBufferInfo>  BufferInfos;
	std::vector<VkDescriptorImageInfo>   ImageInfos;

	std::vector<VkDescriptorSetLayout>   SetLayouts;

	void AddDescriptorSetWrites(VkWriteDescriptorSet descriptorWrite)
	{
		DescriptorSetWrites.emplace_back(descriptorWrite);
	}
	void SetVariableCount(const uint32_t variableCount)
	{
		VariableCount = variableCount;
	}
};

struct DescriptorPoolContext
{
	std::vector<VkDescriptorPoolSize> DescriptorPoolSizes;
	VkDescriptorPoolCreateInfo DescriptorPoolInfo;

	void AddPoolSize(VkDescriptorPoolSize descriptorPoolSize)
	{
		DescriptorPoolSizes.emplace_back(descriptorPoolSize);
	}
};

namespace GG
{
	class DescriptorManager
	{
	public:
		void CreateDescriptorPool(VkDevice device, int maxFramesInFlight, DescriptorPoolContext descriptorPoolContext);
		void CreateDescriptorSets(DescriptorSetsContext descriptorSetsContext, int maxFramesInFlight, VkDevice device);
		void CreateDescriptorSetLayout(VkDevice device, DescriptorSetLayoutContext descriptorSetLayoutContext);

		VkDescriptorSetLayout& GetDescriptorSetLayout(int idx) { return m_DescriptorSetLayout[idx]; }
		std::vector<VkDescriptorSet>& GetDescriptorSets(int idx) { return m_DescriptorSets[idx]; }
		VkDescriptorPool& GetDescriptorPool(int idx) { return m_DescriptorPool[idx]; }

		void Destroy(VkDevice device) const;
	private:
		std::vector <VkDescriptorPool> m_DescriptorPool;
		std::vector<std::vector<VkDescriptorSet>> m_DescriptorSets;
		std::vector <VkDescriptorSetLayout> m_DescriptorSetLayout;
	};
}
