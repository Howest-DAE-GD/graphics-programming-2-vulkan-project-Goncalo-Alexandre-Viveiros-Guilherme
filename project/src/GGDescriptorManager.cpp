#include "GGDescriptorManager.h"

#include <array>
#include <iostream>
#include <stdexcept>

#include "GGBuffer.h"

using namespace GG;

void DescriptorManager::CreateDescriptorPool(VkDevice device,int maxFramesInFlight, DescriptorPoolContext descriptorPoolContext)
{
	m_DescriptorPool.emplace_back();

	if (vkCreateDescriptorPool(device, &descriptorPoolContext.DescriptorPoolInfo, nullptr, &m_DescriptorPool.back()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void DescriptorManager::CreateDescriptorSets(DescriptorSetsContext descriptorSetsContext, int maxFramesInFlight, VkDevice device)
{
	m_DescriptorSets.emplace_back();
	m_DescriptorSets.back().resize(maxFramesInFlight);

	if (vkAllocateDescriptorSets(device, &descriptorSetsContext.AllocateInfo, m_DescriptorSets.back().data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < maxFramesInFlight; i++)
	{
		//VkDescriptorImageInfo imageInfo{};
		//imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		//imageInfo.imageView = imageViews;
		//imageInfo.sampler = sampler;

		for (auto& descriptorWrites : descriptorSetsContext.DescriptorSetWrites)
		{
			descriptorWrites.dstSet = m_DescriptorSets.back()[i];
		}

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorSetsContext.DescriptorSetWrites.size()), descriptorSetsContext.DescriptorSetWrites.data(), 0, nullptr);
	}
}

void DescriptorManager::CreateDescriptorSetLayout(VkDevice device, DescriptorSetLayoutContext descriptorSetLayoutContext)
{
	m_DescriptorSetLayout.emplace_back();
	VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
	bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	bindingFlagsInfo.bindingCount = static_cast<int>(descriptorSetLayoutContext.LayoutBindings.size());
	bindingFlagsInfo.pBindingFlags = descriptorSetLayoutContext.BindingFlags.data();

	VkDescriptorSetLayoutCreateFlags flags = 0;
	flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.pNext = &bindingFlagsInfo;
	layoutInfo.flags = flags;
	layoutInfo.bindingCount = static_cast<uint32_t>(descriptorSetLayoutContext.LayoutBindings.size());
	layoutInfo.pBindings = descriptorSetLayoutContext.LayoutBindings.data();

	int idx = descriptorSetLayoutContext.DescriptorSetLayoutIndex;

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_DescriptorSetLayout[idx]) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void DescriptorManager::Destroy(VkDevice device) const
{
	for (auto& DescriptorPool : m_DescriptorPool)
	{
		vkDestroyDescriptorPool(device, DescriptorPool, nullptr);
	}

	for (auto& descriptorSetLayout: m_DescriptorSetLayout)
	{
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	}

}
