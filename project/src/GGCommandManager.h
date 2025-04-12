#pragma once 
#include <vector>
#include <vulkan/vulkan_core.h>

class Scene;

namespace GG
{
	class SwapChain;

	class CommandManager
	{
	public:
		void CreateCommandPool(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface);
		void CreateCommandBuffers(const VkDevice& device, const int maxFramesInFlight);
		void RecordCommandBuffer(uint32_t imageIndex, SwapChain* swapChain, VkRenderPass renderPass, int currentFrame
		                                , VkPipeline graphicsPipeline, VkPipelineLayout pipelineLayout,Scene* scene, std::vector<VkDescriptorSet>& descriptorSets);

		VkCommandBuffer BeginSingleTimeCommands(VkDevice device) const;
		void EndSingleTimeCommands(const VkQueue& graphicsQueue, const VkCommandBuffer& commandBuffer, const VkDevice& device) const;

		std::vector<VkCommandBuffer>& GetCommandBuffers() { return m_CommandBuffers; }
		VkCommandPool& GetCommandPool() { return m_CommandPool; }
	private:
		VkCommandPool m_CommandPool;
		std::vector<VkCommandBuffer> m_CommandBuffers;
	};
}
