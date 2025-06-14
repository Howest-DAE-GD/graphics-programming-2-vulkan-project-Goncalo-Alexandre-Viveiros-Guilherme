#pragma once 
#include <vector>
#include <vulkan/vulkan_core.h>

namespace GG
{
	class BlitPass;
}

namespace GG
{
	class Image;
	class GBuffer;
	class DescriptorManager;
	class Pipeline;
	class SwapChain;
}

class Scene;

namespace GG
{
	struct PipelinesForCommandBuffer
	{
		Pipeline* prePassPipeline;
		Pipeline* GBufferPipeline;
		Pipeline* lightingPipeline;
		Pipeline* blitPipeline;
	};
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

	class CommandManager
	{
	public:
		void CreateCommandPool(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface);
		void CreateCommandBuffers(const VkDevice& device, const int maxFramesInFlight);
		void RecordCommandBuffer(uint32_t imageIndex, SwapChain* swapChain, int currentFrame, GBuffer& gBuffer, BlitPass& blitPass,
			PipelinesForCommandBuffer pipelines,Scene* scene, DescriptorManager* descriptorManager);

		void DrawScene(SwapChain* swapChain, const std::vector<VkDescriptorSet>& descriptorSets, int currentFrame, Pipeline* pipeline, Scene* scene) const;

		VkCommandBuffer BeginSingleTimeCommands(VkDevice device) const;
		void EndSingleTimeCommands(const VkQueue& graphicsQueue, const VkCommandBuffer& commandBuffer, const VkDevice& device) const;
		void TransitionImage(Image& image, TransitionImgContext context, int currentFrame) const;
		void TransitionImage(VkImage image, TransitionImgContext context, int currentFrame);

		void QuickDebug(SwapChain* swapChain, GBuffer& gBuffer, int currentFrame, uint32_t imageIndex);

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
