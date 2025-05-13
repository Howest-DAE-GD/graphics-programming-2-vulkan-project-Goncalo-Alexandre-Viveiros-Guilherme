#pragma once
#include <string>
#include <vulkan/vulkan_core.h>

class Scene;

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

		VkPipelineLayout GetPipelineLayout() { return pipelineLayout; }
		VkPipeline GetPipeline() { return graphicsPipeline; }

		void Destroy(VkDevice device) const;
	private:
		VkPipelineLayout pipelineLayout;
		VkPipeline graphicsPipeline;
	};
}
