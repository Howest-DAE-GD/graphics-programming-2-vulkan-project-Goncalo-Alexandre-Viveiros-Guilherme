#pragma once
#include "GGGBuffer.h"

#include "GGBuffer.h"
#include "GGDescriptorManager.h"
#include "GGShader.h"
#include "GGVkDevice.h"
#include "GGVkHelperFunctions.h"

GG::GBuffer::GBuffer(): m_AlbedoImage(), m_NormalMapImage(), m_MettalicRoughnessImage()
{
	m_Pipeline = new Pipeline();
}

void GG::GBuffer::CreateImages(VkExtent2D swapChainExtent, Device* device)
{
	m_AlbedoImage.CreateImage(swapChainExtent.width, swapChainExtent.height, 1, device->GetMssaSamples(),
		VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, device->GetVulkanDevice(), device->GetVulkanPhysicalDevice());

	m_AlbedoImage.CreateImageView(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, 1, device->GetVulkanDevice());

	m_NormalMapImage.CreateImage(swapChainExtent.width, swapChainExtent.height, 1, device->GetMssaSamples(),
		VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, device->GetVulkanDevice(), device->GetVulkanPhysicalDevice());

	m_NormalMapImage.CreateImageView(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, 1, device->GetVulkanDevice());

	m_MettalicRoughnessImage.CreateImage(swapChainExtent.width, swapChainExtent.height, 1, device->GetMssaSamples(),
		VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, device->GetVulkanDevice(), device->GetVulkanPhysicalDevice());

	m_MettalicRoughnessImage.CreateImageView(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, 1, device->GetVulkanDevice());
}

GG::Image& GG::GBuffer::GetAlbedoGGImage()
{
	return m_AlbedoImage;
}

GG::Image& GG::GBuffer::GetNormalMapGGImage()
{
	return m_NormalMapImage;
}

GG::Image& GG::GBuffer::GetMettalicRoughnessGGImage()
{
	return m_MettalicRoughnessImage;
}

void GG::GBuffer::CreatePipeline(Device* device, DescriptorManager* descriptorManager)
{
	PipelineContext graphicsPipelineContext{};

	GG::Shader vertShader{ "shaders/shader.vert.spv" , device->GetVulkanDevice() };
	GG::Shader fragShader{ "shaders/shader.frag.spv" , device->GetVulkanDevice() };

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShader.GetShaderModule();
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShader.GetShaderModule();
	fragShaderStageInfo.pName = "main";

	graphicsPipelineContext.ShaderStages = { vertShaderStageInfo, fragShaderStageInfo };

	graphicsPipelineContext.MultisampleState.rasterizationSamples = device->GetMssaSamples();

	graphicsPipelineContext.ColorAttachmentFormats.emplace_back(GetAlbedoGGImage().GetImageFormat());
	graphicsPipelineContext.ColorAttachmentFormats.emplace_back(GetNormalMapGGImage().GetImageFormat());
	graphicsPipelineContext.ColorAttachmentFormats.emplace_back(GetMettalicRoughnessGGImage().GetImageFormat());

	static std::array<VkPipelineColorBlendAttachmentState, 3> gBufferBlendAttachmentStates;
	for (auto& state : gBufferBlendAttachmentStates) {
		state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		state.blendEnable = VK_FALSE;
	}

	graphicsPipelineContext.ColorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	graphicsPipelineContext.ColorBlendState.logicOpEnable = VK_FALSE;
	graphicsPipelineContext.ColorBlendState.attachmentCount = static_cast<uint32_t>(gBufferBlendAttachmentStates.size());
	graphicsPipelineContext.ColorBlendState.pAttachments = gBufferBlendAttachmentStates.data();

	graphicsPipelineContext.DepthAttachmentFormat = GG::VkHelperFunctions::FindDepthFormat(device->GetVulkanPhysicalDevice());

	m_Pipeline->CreatePipeline(device->GetVulkanDevice(), descriptorManager->GetDescriptorSetLayout(1), graphicsPipelineContext);

}

void GG::GBuffer::CreateDescriptorSets(Scene* currentScene, Device* device, DescriptorManager* descriptorManager,Buffer* buffer, int maxFramesInFlight)
{
	DescriptorSetsContext descriptorSetsContext;

	descriptorSetsContext.VariableCount = static_cast<uint32_t>(currentScene->GetImageViews().size());

	descriptorSetsContext.ImageInfos.resize(descriptorSetsContext.VariableCount);

	for (uint32_t i = 0; i < descriptorSetsContext.VariableCount; ++i) {
		auto& imageInfos = descriptorSetsContext.ImageInfos[i];
		imageInfos.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfos.imageView = currentScene->GetImageViews()[i];
		imageInfos.sampler = device->GetTextureSampler();
	}

	descriptorSetsContext.BufferInfos.resize(maxFramesInFlight);

	for (size_t i = 0; i < maxFramesInFlight; i++)
	{
		auto& bufferInfo = descriptorSetsContext.BufferInfos[i];
		bufferInfo.buffer = buffer->GetUniformBuffers()[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkWriteDescriptorSet uniformBufferDescriptor;
		uniformBufferDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		uniformBufferDescriptor.pNext = nullptr;
		uniformBufferDescriptor.dstSet = VK_NULL_HANDLE;
		uniformBufferDescriptor.dstBinding = 0;
		uniformBufferDescriptor.dstArrayElement = 0;
		uniformBufferDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBufferDescriptor.descriptorCount = 1;
		uniformBufferDescriptor.pBufferInfo = &descriptorSetsContext.BufferInfos[i];

		VkWriteDescriptorSet samplerDescriptor;
		samplerDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		samplerDescriptor.pNext = nullptr;
		samplerDescriptor.dstSet = VK_NULL_HANDLE;
		samplerDescriptor.dstBinding = 1;
		samplerDescriptor.dstArrayElement = 0;
		samplerDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		samplerDescriptor.descriptorCount = 1;
		samplerDescriptor.pImageInfo = descriptorSetsContext.ImageInfos.data();

		VkWriteDescriptorSet imagesDescriptor;
		imagesDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		imagesDescriptor.pNext = nullptr;
		imagesDescriptor.dstSet = VK_NULL_HANDLE;
		imagesDescriptor.dstBinding = 2;
		imagesDescriptor.dstArrayElement = 0;
		imagesDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		imagesDescriptor.descriptorCount = descriptorSetsContext.VariableCount;
		imagesDescriptor.pImageInfo = descriptorSetsContext.ImageInfos.data();

		descriptorSetsContext.AddDescriptorSetWrites(uniformBufferDescriptor);
		descriptorSetsContext.AddDescriptorSetWrites(samplerDescriptor);
		descriptorSetsContext.AddDescriptorSetWrites(imagesDescriptor);
	}

	std::vector<uint32_t> descriptorCounts(maxFramesInFlight, descriptorSetsContext.VariableCount);

	VkDescriptorSetVariableDescriptorCountAllocateInfo variableCountInfo{};
	variableCountInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
	variableCountInfo.descriptorSetCount = static_cast<uint32_t>(maxFramesInFlight);
	variableCountInfo.pDescriptorCounts = descriptorCounts.data();

	descriptorSetsContext.SetLayouts.assign(
		maxFramesInFlight,
		descriptorManager->GetDescriptorSetLayout(1)
	);

	// 2) zero-init and fill AllocateInfo
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.pNext = &variableCountInfo;
	allocInfo.descriptorPool = descriptorManager->GetDescriptorPool(1);
	allocInfo.descriptorSetCount = static_cast<uint32_t>(descriptorSetsContext.SetLayouts.size());
	allocInfo.pSetLayouts = descriptorSetsContext.SetLayouts.data();

	descriptorSetsContext.AllocateInfo = allocInfo;
	descriptorSetsContext.DescriptorSetLayout = descriptorManager->GetDescriptorSetLayout(1);
	descriptorManager->CreateDescriptorSets(std::move(descriptorSetsContext), maxFramesInFlight, device->GetVulkanDevice());
}

void GG::GBuffer::CreateDescriptorSetLayout(Device* device, DescriptorManager* descriptorManager)
{
	DescriptorSetLayoutContext descriptorSetLayoutContext;

	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkPhysicalDeviceLimits limits = GG::VkHelperFunctions::FindPhysicalDeviceLimits(device->GetVulkanPhysicalDevice());

	VkDescriptorSetLayoutBinding storageImageBinding{};
	storageImageBinding.binding = 2;
	storageImageBinding.descriptorCount = limits.maxPerStageDescriptorSampledImages;
	storageImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	storageImageBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	storageImageBinding.pImmutableSamplers = nullptr;


	std::vector<VkDescriptorBindingFlags> bindingFlags = {
		0,                                                         // binding 0
		0,                                                         // binding 1 (no special flags)
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
		| VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT       // binding 2 (highest)
		| VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
	};

	descriptorSetLayoutContext.AddDescriptorSetLayout(uboLayoutBinding);
	descriptorSetLayoutContext.AddDescriptorSetLayout(samplerLayoutBinding);
	descriptorSetLayoutContext.AddDescriptorSetLayout(storageImageBinding);
	descriptorSetLayoutContext.BindingFlags = bindingFlags;
	descriptorSetLayoutContext.DescriptorSetLayoutIndex = 1;

	descriptorManager->CreateDescriptorSetLayout(device->GetVulkanDevice(), std::move(descriptorSetLayoutContext));
}

void GG::GBuffer::CreateDescriptorPool(Device* device, DescriptorManager* descriptorManager, int maxFramesInFlight)
{
	VkPhysicalDeviceLimits limits = GG::VkHelperFunctions::FindPhysicalDeviceLimits(device->GetVulkanPhysicalDevice());
	std::vector<VkDescriptorPoolSize> poolSizes{ 3 };
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(maxFramesInFlight);
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(maxFramesInFlight);
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	poolSizes[2].descriptorCount = limits.maxPerStageDescriptorSampledImages;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(maxFramesInFlight);

	DescriptorPoolContext poolContext;
	poolContext.DescriptorPoolInfo = poolInfo;
	poolContext.DescriptorPoolSizes = poolSizes;

	descriptorManager->CreateDescriptorPool(device->GetVulkanDevice(), maxFramesInFlight, std::move(poolContext));
}

void GG::GBuffer::CleanUp(VkDevice device) const
{
	m_AlbedoImage.DestroyImg(device);
	m_NormalMapImage.DestroyImg(device);
	m_MettalicRoughnessImage.DestroyImg(device);

	m_Pipeline->Destroy(device);
}
