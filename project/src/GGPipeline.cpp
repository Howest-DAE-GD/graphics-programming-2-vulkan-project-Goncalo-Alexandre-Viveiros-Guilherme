#include <stdexcept>
#include <vector>

#include "GGPipeLine.h"
#include "GGSwapChain.h"
#include "GGVkHelperFunctions.h"
#include "Model.h"
#include "Scene.h"

using namespace GG;

void Pipeline::CreatePipeline(VkDevice& device, VkDescriptorSetLayout& descriptorSetLayout, PipelineContext pipelineContext)
{
	m_PipeLineStageFlags = pipelineContext.PushConstantRange.stageFlags;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pipelineContext.PushConstantRange;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkPipelineRenderingCreateInfo pipeline_create{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR };
	pipeline_create.pNext = VK_NULL_HANDLE;
	pipeline_create.colorAttachmentCount = pipelineContext.ColorAttachmentCount;
	pipeline_create.pColorAttachmentFormats = pipelineContext.ColorAttachmentFormat;
	pipeline_create.depthAttachmentFormat = pipelineContext.DepthAttachmentFormat;
	pipeline_create.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = &pipeline_create;
	pipelineInfo.renderPass = VK_NULL_HANDLE;

	pipelineInfo.stageCount = pipelineContext.ShaderStages.size();
	pipelineInfo.pStages = pipelineContext.ShaderStages.data();

	pipelineInfo.pVertexInputState = &pipelineContext.VertexInputState;
	pipelineInfo.pInputAssemblyState = &pipelineContext.InputAssemblyState;
	pipelineInfo.pViewportState = &pipelineContext.ViewportState;
	pipelineInfo.pRasterizationState = &pipelineContext.RasterizerState;
	pipelineInfo.pMultisampleState = &pipelineContext.MultisampleState;
	pipelineInfo.pDepthStencilState = &pipelineContext.DepthStencilState;
	pipelineInfo.pColorBlendState = &pipelineContext.ColorBlendState;
	pipelineInfo.pDynamicState = &pipelineContext.DynamicState;

	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.subpass = 0;

	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create graphics pipeline!");
	}
}

void Pipeline::Destroy(VkDevice device) const
{
	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}

PipelineContext::PipelineContext()
{
	ShaderStages = {};

	VertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	VertexInputState.pNext = nullptr;
	DefaultBindingDescription = Vertex::getBindingDescription();
	DefaultAttributeDescriptions = Vertex::getAttributeDescriptions();

	VertexInputState.vertexBindingDescriptionCount = 1;
	VertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(DefaultAttributeDescriptions.size());
	VertexInputState.pVertexBindingDescriptions = &DefaultBindingDescription;
	VertexInputState.pVertexAttributeDescriptions = DefaultAttributeDescriptions.data();

	InputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	InputAssemblyState.pNext = nullptr;
	InputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	InputAssemblyState.primitiveRestartEnable = VK_FALSE;

	ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	ViewportState.pNext = nullptr;
	ViewportState.viewportCount = 1;
	ViewportState.scissorCount = 1;

	RasterizerState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	RasterizerState.pNext = nullptr;
	RasterizerState.depthClampEnable = VK_FALSE;

	RasterizerState.rasterizerDiscardEnable = VK_FALSE;

	RasterizerState.polygonMode = VK_POLYGON_MODE_FILL;
	RasterizerState.lineWidth = 1.0f;
	RasterizerState.cullMode = VK_CULL_MODE_BACK_BIT;
	RasterizerState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	RasterizerState.depthBiasEnable = VK_FALSE;
	RasterizerState.depthBiasConstantFactor = 0.0f; // Optional
	RasterizerState.depthBiasClamp = 0.0f; // Optional
	RasterizerState.depthBiasSlopeFactor = 0.0f; // Optional

	MultisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	MultisampleState.pNext = nullptr;
	MultisampleState.sampleShadingEnable = VK_TRUE; // enable sample shading in the pipeline
	MultisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	MultisampleState.minSampleShading = .2f; // min fraction for sample shading; closer to one is smooth
	MultisampleState.pSampleMask = nullptr; // Optional
	MultisampleState.alphaToCoverageEnable = VK_FALSE; // Optional
	MultisampleState.alphaToOneEnable = VK_FALSE; // Optional

	DepthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	DepthStencilState.pNext = nullptr;
	DepthStencilState.depthTestEnable = VK_TRUE;
	DepthStencilState.depthWriteEnable = VK_FALSE;
	DepthStencilState.depthCompareOp = VK_COMPARE_OP_EQUAL;
	DepthStencilState.depthBoundsTestEnable = VK_FALSE;
	DepthStencilState.stencilTestEnable = VK_FALSE;

	DefaultColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	DefaultColorBlendAttachment.blendEnable = VK_FALSE;

	ColorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	ColorBlendState.pNext = nullptr;
	ColorBlendState.logicOpEnable = VK_FALSE;
	ColorBlendState.logicOp = VK_LOGIC_OP_COPY; // Optional
	ColorBlendState.attachmentCount = 1;
	ColorBlendState.pAttachments = &DefaultColorBlendAttachment;
	ColorBlendState.blendConstants[0] = 0.0f; // Optional
	ColorBlendState.blendConstants[1] = 0.0f; // Optional
	ColorBlendState.blendConstants[2] = 0.0f; // Optional
	ColorBlendState.blendConstants[3] = 0.0f; // Optional

	DefaultDynamicStates =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	DynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	DynamicState.pNext = nullptr;
	DynamicState.dynamicStateCount = static_cast<uint32_t>(DefaultDynamicStates.size());
	DynamicState.pDynamicStates = DefaultDynamicStates.data();

	VkShaderStageFlags pipelineStageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	PushConstantRange.stageFlags = pipelineStageFlags;
	PushConstantRange.offset = 0;
	PushConstantRange.size = sizeof(PushConstants);
}
