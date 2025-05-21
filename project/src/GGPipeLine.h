#pragma once
#include <string>
#include <vulkan/vulkan_core.h>
#include <glm/gtc/matrix_transform.hpp>


class Scene;

struct alignas(16) PushConstants
{
	glm::mat4 modelMatrix;
	uint32_t materialIndex;
};
namespace GG
{
	class SwapChain;

	class Pipeline
	{
	public:
		static std::vector<char> ReadFile(const std::string& filename);
		static VkShaderModule CreateShaderModule(const std::vector<char>& code, VkDevice& device);
		void CreateGraphicsPipeline(VkDevice& device, const VkPhysicalDevice& physicalDevice, VkSampleCountFlagBits& mssaSamples, 
			VkDescriptorSetLayout& descriptorSetLayout, SwapChain* swapchain, Scene* scene);

		void CreateDepthOnlyPipeline(VkDevice& device, const VkPhysicalDevice& physicalDevice, VkSampleCountFlagBits& mssaSamples,
			VkDescriptorSetLayout& descriptorSetLayout, SwapChain* swapchain, Scene* scene);

		VkPipelineLayout GetPipelineLayout() { return pipelineLayout; }
		VkPipeline GetPipeline() { return graphicsPipeline; }

		void Destroy(VkDevice device) const;
	private:
		VkPipelineLayout pipelineLayout;
		VkPipeline graphicsPipeline;
	};
}
