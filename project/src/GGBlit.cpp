#include "GGBlit.h"

#include "GGDescriptorManager.h"
#include "GGImage.h"
#include "GGShader.h"
#include "GGVkDevice.h"
#include "GGVkHelperFunctions.h"

GG::BlitPass::BlitPass(): m_Image(nullptr)
{
	m_Pipeline = new Pipeline();
}

void GG::BlitPass::CreateImage(VkExtent2D swapChainExtent, Device* device)
{
	m_Image = new Image();
	m_Image->CreateImage(swapChainExtent.width, swapChainExtent.height, 1, device->GetMssaSamples(),
		VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, device->GetVulkanDevice(), device->GetVulkanPhysicalDevice());

	m_Image->CreateImageView(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, 1, device->GetVulkanDevice());
}

void GG::BlitPass::CreateDescriptorSets(Device* device, DescriptorManager* descriptorManager, int maxFramesInFlight)
{
	DescriptorSetsContext descriptorSetsContext;

	VkDescriptorImageInfo imageInfo;

	imageInfo.sampler = device->GetTextureSampler();
	imageInfo.imageView = m_Image->GetImageView();
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	VkWriteDescriptorSet inColor = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstBinding = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo = &imageInfo
	};

	descriptorSetsContext.AddDescriptorSetWrites(inColor);

	descriptorSetsContext.SetLayouts.assign(
		maxFramesInFlight,
		descriptorManager->GetDescriptorSetLayout(3)
	);

	VkDescriptorSetAllocateInfo allocInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descriptorManager->GetDescriptorPool(3),
		.descriptorSetCount = static_cast<uint32_t>(maxFramesInFlight),
		.pSetLayouts = descriptorSetsContext.SetLayouts.data()
	};

	descriptorSetsContext.AllocateInfo = allocInfo;
	descriptorSetsContext.DescriptorSetLayout = descriptorManager->GetDescriptorSetLayout(3);

	descriptorManager->CreateDescriptorSets(std::move(descriptorSetsContext), maxFramesInFlight, device->GetVulkanDevice());
}

void GG::BlitPass::CreateDescriptorPool(Device* device, DescriptorManager* descriptorManager, int maxFramesInFlight)
{
	std::vector<VkDescriptorPoolSize> poolSizes(1);
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[0].descriptorCount = maxFramesInFlight;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = maxFramesInFlight;

	DescriptorPoolContext poolContext;
	poolContext.DescriptorPoolInfo = poolInfo;
	for (const auto& size : poolSizes)
		poolContext.AddPoolSize(size);

	descriptorManager->CreateDescriptorPool(device->GetVulkanDevice(), maxFramesInFlight, std::move(poolContext));
}

void GG::BlitPass::CreateDescriptorSetLayout(Device* device, DescriptorManager* descriptorManager)
{
	DescriptorSetLayoutContext descriptorSetLayoutContext;

	// Combined image samplers
	VkDescriptorSetLayoutBinding inColor = {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};

	descriptorSetLayoutContext.AddDescriptorSetLayout(inColor);

	descriptorSetLayoutContext.DescriptorSetLayoutIndex = 3;
	descriptorSetLayoutContext.BindingFlags = { 0 }; // No special flags needed

	descriptorManager->CreateDescriptorSetLayout(device->GetVulkanDevice(), std::move(descriptorSetLayoutContext));
}

void GG::BlitPass::CreateBlitPipeline(Device* device, DescriptorManager* descriptorManager,VkFormat swapchainFormat) const
{
	PipelineContext BlitPipelineContext{};

	GG::Shader fragShader{ "shaders/blitShader.frag.spv" , device->GetVulkanDevice() };
	GG::Shader vertShader{ "shaders/lightShader.vert.spv" , device->GetVulkanDevice() };

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShader.GetShaderModule();
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShader.GetShaderModule();
	vertShaderStageInfo.pName = "main";

	BlitPipelineContext.ShaderStages = { fragShaderStageInfo,vertShaderStageInfo };

	BlitPipelineContext.VertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	BlitPipelineContext.VertexInputState.vertexBindingDescriptionCount = 0;
	BlitPipelineContext.VertexInputState.pVertexBindingDescriptions = nullptr;
	BlitPipelineContext.VertexInputState.vertexAttributeDescriptionCount = 0;
	BlitPipelineContext.VertexInputState.pVertexAttributeDescriptions = nullptr;

	BlitPipelineContext.InputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	BlitPipelineContext.InputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	BlitPipelineContext.InputAssemblyState.primitiveRestartEnable = VK_FALSE;

	BlitPipelineContext.RasterizerState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	BlitPipelineContext.RasterizerState.depthClampEnable = VK_FALSE;
	BlitPipelineContext.RasterizerState.rasterizerDiscardEnable = VK_FALSE;
	BlitPipelineContext.RasterizerState.polygonMode = VK_POLYGON_MODE_FILL;
	BlitPipelineContext.RasterizerState.cullMode = VK_CULL_MODE_NONE;
	BlitPipelineContext.RasterizerState.frontFace = VK_FRONT_FACE_CLOCKWISE;
	BlitPipelineContext.RasterizerState.lineWidth = 1.0f;

	BlitPipelineContext.ColorAttachmentFormats.emplace_back(swapchainFormat);

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	BlitPipelineContext.ColorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	BlitPipelineContext.ColorBlendState.logicOpEnable = VK_FALSE;
	BlitPipelineContext.ColorBlendState.attachmentCount = 1;
	BlitPipelineContext.ColorBlendState.pAttachments = &colorBlendAttachment;

	BlitPipelineContext.DepthStencilState.depthTestEnable = VK_FALSE;
	BlitPipelineContext.DepthStencilState.depthWriteEnable = VK_FALSE;
	BlitPipelineContext.DepthStencilState.stencilTestEnable = VK_FALSE;

	BlitPipelineContext.DepthAttachmentFormat = GG::VkHelperFunctions::FindDepthFormat(device->GetVulkanPhysicalDevice());

	BlitPipelineContext.MultisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	BlitPipelineContext.MultisampleState.rasterizationSamples = device->GetMssaSamples();
	BlitPipelineContext.MultisampleState.sampleShadingEnable = VK_FALSE;

	m_Pipeline->CreatePipeline(device->GetVulkanDevice(), descriptorManager->GetDescriptorSetLayout(3), BlitPipelineContext);
}


void GG::BlitPass::Cleanup(Device* device) const
{
	m_Image->DestroyImg(device->GetVulkanDevice());
	delete m_Image;

	m_Pipeline->Destroy(device->GetVulkanDevice());
}
