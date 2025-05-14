#pragma once 
#include <vector>
#include <vulkan/vulkan_core.h>

class Scene;

namespace GG
{
	struct TransitionImgContext
	{
		VkImageLayout oldLayout;
		VkImageLayout newLayout;
		VkImageAspectFlags aspectMask;
		VkAccessFlags srcAccessMask;
		VkAccessFlags dstAccessMask;
		VkPipelineStageFlags srcStage;
		VkPipelineStageFlags dstStage;
	};
	class Pipeline;
	class SwapChain;

	class CommandManager
	{
	public:
		void CreateCommandPool(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface);
		void CreateCommandBuffers(const VkDevice& device, const int maxFramesInFlight);
		void RecordCommandBuffer(uint32_t imageIndex, SwapChain* swapChain, int currentFrame, Pipeline* pipeline,Scene* scene, std::vector<VkDescriptorSet>& descriptorSets);
		void DrawScene(SwapChain* swapChain, const std::vector<VkDescriptorSet>& descriptorSets, int currentFrame, Pipeline* pipeline, Scene* scene) const;

		VkCommandBuffer BeginSingleTimeCommands(VkDevice device) const;
		void EndSingleTimeCommands(const VkQueue& graphicsQueue, const VkCommandBuffer& commandBuffer, const VkDevice& device) const;
		void TransitionImage(VkImage image, TransitionImgContext context, int currentFrame) const;

		std::vector<VkCommandBuffer>& GetCommandBuffers() { return m_CommandBuffers; }
		VkCommandPool& GetCommandPool() { return m_CommandPool; }

		void Destroy(VkDevice device) const;
	private:
		VkCommandPool m_CommandPool;
		std::vector<VkCommandBuffer> m_CommandBuffers;
		VkImageLayout currentImagesLayouts { VK_IMAGE_LAYOUT_UNDEFINED };

		PFN_vkCmdBeginRenderingKHR vkCmdBeginRenderingKHR = nullptr;
		PFN_vkCmdEndRenderingKHR   vkCmdEndRenderingKHR = nullptr;
	};
}
