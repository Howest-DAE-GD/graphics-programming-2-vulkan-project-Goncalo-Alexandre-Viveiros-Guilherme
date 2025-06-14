#pragma once
#include <array>
#include <vulkan/vulkan_core.h>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>


class Scene;

struct alignas(16) PushConstants
{
	glm::mat4 ModelMatrix;
	uint32_t AlbedoTexIndex;
	uint32_t AOTexIndex;
	uint32_t NormalMapIndex;
	uint32_t MetallicRoughnessMapIndex;
};

struct PipelineContext
{
	PipelineContext();

	std::vector<VkPipelineShaderStageCreateInfo> ShaderStages{};
	VkPushConstantRange PushConstantRange{};
	std::vector<VkFormat> ColorAttachmentFormats	{ };
	VkFormat DepthAttachmentFormat					{ VK_FORMAT_UNDEFINED };
	VkFormat StencilAttachmentFormat				{ VK_FORMAT_UNDEFINED };

	VkPipelineVertexInputStateCreateInfo    VertexInputState{};
	VkPipelineInputAssemblyStateCreateInfo	InputAssemblyState{};
	VkPipelineViewportStateCreateInfo		ViewportState{};
	VkPipelineRasterizationStateCreateInfo	RasterizerState{};
	VkPipelineMultisampleStateCreateInfo	MultisampleState{};
	VkPipelineDepthStencilStateCreateInfo	DepthStencilState{};
	VkPipelineColorBlendStateCreateInfo		ColorBlendState{};
	VkPipelineDynamicStateCreateInfo		DynamicState{};
	std::vector<VkVertexInputAttributeDescription> AttributeDescriptions{};

private:
	std::vector<VkDynamicState> DefaultDynamicStates{};
	VkPipelineColorBlendAttachmentState DefaultColorBlendAttachment{};
	VkVertexInputBindingDescription DefaultBindingDescription{};

};
namespace GG
{
	class SwapChain;

	class Pipeline
	{
	public:
		void CreatePipeline(VkDevice& device, VkDescriptorSetLayout& descriptorSetLayout,PipelineContext pipelineContext);

		VkPipelineLayout GetPipelineLayout() { return pipelineLayout; }
		VkPipeline GetPipeline() { return graphicsPipeline; }
		VkShaderStageFlags GetStageFlags() { return m_PipeLineStageFlags; }

		void Destroy(VkDevice device) const;
	private:
		VkPipelineLayout pipelineLayout;
		VkPipeline graphicsPipeline;
		VkShaderStageFlags m_PipeLineStageFlags;
	};
}
