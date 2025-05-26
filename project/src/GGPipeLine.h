#pragma once
#include <array>
#include <vulkan/vulkan_core.h>
#include <glm/gtc/matrix_transform.hpp>


class Scene;

struct alignas(16) PushConstants
{
	glm::mat4 modelMatrix;
	uint32_t materialIndex;
};

struct PipelineContext
{
	PipelineContext();

	std::vector<VkPipelineShaderStageCreateInfo> ShaderStages{};
	VkPushConstantRange PushConstantRange{};
	VkFormat* ColorAttachmentFormat			{ nullptr };
	uint32_t ColorAttachmentCount			{ 0 };
	VkFormat DepthAttachmentFormat			{ VK_FORMAT_UNDEFINED };
	VkFormat StencilAttachmentFormat		{ VK_FORMAT_UNDEFINED };

	VkPipelineVertexInputStateCreateInfo    VertexInputState{};
	VkPipelineInputAssemblyStateCreateInfo	InputAssemblyState{};
	VkPipelineViewportStateCreateInfo		ViewportState{};
	VkPipelineRasterizationStateCreateInfo	RasterizerState{};
	VkPipelineMultisampleStateCreateInfo	MultisampleState{};
	VkPipelineDepthStencilStateCreateInfo	DepthStencilState{};
	VkPipelineColorBlendStateCreateInfo		ColorBlendState{};
	VkPipelineDynamicStateCreateInfo		DynamicState{};

private:
	std::vector<VkDynamicState> DefaultDynamicStates{};
	VkPipelineColorBlendAttachmentState DefaultColorBlendAttachment{};
	std::array<VkVertexInputAttributeDescription, 3> DefaultAttributeDescriptions{};
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
