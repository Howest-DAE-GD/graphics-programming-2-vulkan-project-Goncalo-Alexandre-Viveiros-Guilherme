#include "GGCommandManager.h"

#include <array>
#include <stdexcept>

#include "GGBuffer.h"
#include "GGDescriptorManager.h"
#include "GGPipeLine.h"
#include "GGSwapChain.h"
#include "GGTexture.h"
#include "GGVkHelperFunctions.h"
#include "Scene.h"

using namespace GG;

void CommandManager::CreateCommandPool(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface)
{
	QueueFamilyIndices queueFamilyIndices = GG::VkHelperFunctions::FindQueueFamilies(physicalDevice, surface);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	if (vkCreateCommandPool(device, &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create command pool!");
	}
}

void CommandManager::CreateCommandBuffers(const VkDevice& device, const int maxFramesInFlight)
{
	m_CommandBuffers.resize(maxFramesInFlight);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_CommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();

	if (vkAllocateCommandBuffers(device, &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

void CommandManager::RecordCommandBuffer(uint32_t imageIndex, SwapChain* swapChain, int currentFrame, GBuffer gBuffer,
	Pipeline* pipeline, Pipeline* prepassPipeline, Scene* scene, DescriptorManager* descriptorManager)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(m_CommandBuffers[currentFrame], &beginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	TransitionImgContext optimalColorDraw{ VK_IMAGE_LAYOUT_UNDEFINED ,
										   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
													VK_IMAGE_ASPECT_COLOR_BIT ,
										0 ,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
													VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	TransitionImgContext optimalDepthDraw = optimalColorDraw;
	optimalDepthDraw.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	optimalDepthDraw.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	TransitionImage(swapChain->GetSwapChainImages()[imageIndex], optimalColorDraw,currentFrame);
	TransitionImage(swapChain->GetDepthImage(), optimalDepthDraw,currentFrame);

	// 2) --- DEPTH PRE-PASS ---

	VkRenderingAttachmentInfo depth_attachment_info{};
	depth_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
	depth_attachment_info.pNext = nullptr;
	depth_attachment_info.imageView = swapChain->GetDepthImageView();
	depth_attachment_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR;
	depth_attachment_info.resolveMode = VK_RESOLVE_MODE_NONE;
	depth_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depth_attachment_info.clearValue.depthStencil = { 1.0f, 0 };


	VkRenderingInfo depthPassInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO };
	depthPassInfo.renderArea = { {0,0}, swapChain->GetSwapChainExtent() };
	depthPassInfo.layerCount = 1;
	depthPassInfo.colorAttachmentCount = 0;     // no color
	depthPassInfo.pDepthAttachment = &depth_attachment_info;

	vkCmdBeginRendering(m_CommandBuffers[currentFrame], &depthPassInfo);

	vkCmdBindPipeline(m_CommandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS,
		prepassPipeline->GetPipeline());
	vkCmdBindDescriptorSets(m_CommandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS,
		prepassPipeline->GetPipelineLayout(),
		0, 1, &descriptorManager->GetDescriptorSets(0)[currentFrame],
		0, nullptr);

	DrawScene(swapChain, descriptorManager->GetDescriptorSets(0), currentFrame, prepassPipeline, scene);

	vkCmdEndRendering(m_CommandBuffers[currentFrame]);

	// 2) --- G BUFFER ---

	TransitionImgContext albedoToColorAttach{
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_ASPECT_COLOR_BIT,
		0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};

	TransitionImage(gBuffer.GetAlbedoGGImage(), albedoToColorAttach, currentFrame);

	VkRenderingAttachmentInfo gbuffer_albedo_attachment_info{};
	gbuffer_albedo_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
	gbuffer_albedo_attachment_info.pNext = nullptr;
	gbuffer_albedo_attachment_info.imageView = gBuffer.GetAlbedoImageView(); 
	gbuffer_albedo_attachment_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	gbuffer_albedo_attachment_info.resolveMode = VK_RESOLVE_MODE_NONE; 
	gbuffer_albedo_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; 
	gbuffer_albedo_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	gbuffer_albedo_attachment_info.clearValue.color = { {0.0f, 0.0f, 0.0f, 1.0f} };

	VkRenderingAttachmentInfo gbuffer_depth_attachment_info{};
	gbuffer_depth_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
	gbuffer_depth_attachment_info.pNext = nullptr;
	gbuffer_depth_attachment_info.imageView = swapChain->GetDepthImageView();
	gbuffer_depth_attachment_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR;
	gbuffer_depth_attachment_info.resolveMode = VK_RESOLVE_MODE_NONE;
	gbuffer_depth_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	gbuffer_depth_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE; 
	gbuffer_depth_attachment_info.clearValue.depthStencil = { 1.0f, 0 };

	std::vector<VkRenderingAttachmentInfo> colorAttachmentsInfo { gbuffer_albedo_attachment_info };

	VkRect2D render_area = VkRect2D{ VkOffset2D{}, swapChain->GetSwapChainExtent() };
	VkRenderingInfo render_info {};
	render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	render_info.renderArea = render_area;
	render_info.colorAttachmentCount = colorAttachmentsInfo.size();
	render_info.pColorAttachments = colorAttachmentsInfo.data();
	render_info.layerCount = 1;
	render_info.pDepthAttachment = &gbuffer_depth_attachment_info;
	render_info.pStencilAttachment = nullptr;


	for (auto& tex : scene->GetTextures()) {
		TransitionImgContext toRead{
			tex->GetImageLayout(),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
		};
		TransitionImage(tex->GetGGImage(), toRead, currentFrame);
	}

	vkCmdBeginRendering(m_CommandBuffers[currentFrame], &render_info);

	vkCmdBindPipeline(m_CommandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipeline());

	DrawScene(swapChain, descriptorManager->GetDescriptorSets(1), currentFrame, pipeline, scene);

	vkCmdEndRendering(m_CommandBuffers[currentFrame]);

	for (auto& tex : scene->GetTextures()) {
		TransitionImgContext toRead{
			tex->GetImageLayout(),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
		};
		TransitionImage(tex->GetGGImage(), toRead, currentFrame);
	}

	TransitionImgContext presentColorContext = optimalColorDraw;
	presentColorContext.srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	presentColorContext.dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	presentColorContext.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	presentColorContext.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	presentColorContext.oldLayout = optimalColorDraw.newLayout;
	presentColorContext.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	TransitionImage(swapChain->GetSwapChainImages()[imageIndex], presentColorContext, currentFrame);

	if (vkEndCommandBuffer(m_CommandBuffers[currentFrame]) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to record command buffer!");
	}
}

void CommandManager::DrawScene(SwapChain* swapChain, const std::vector<VkDescriptorSet>& descriptorSets, int currentFrame, Pipeline* pipeline, Scene* scene) const
{
	const auto& swapChainExtent = swapChain->GetSwapChainExtent();

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapChainExtent.width);
	viewport.height = static_cast<float>(swapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(m_CommandBuffers[currentFrame], 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;
	vkCmdSetScissor(m_CommandBuffers[currentFrame], 0, 1, &scissor);

	vkCmdBindDescriptorSets(m_CommandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipelineLayout(), 0, 1,
		&descriptorSets[currentFrame], 0, nullptr);

	for (auto& mesh : scene->GetMeshes())
	{
		PushConstants pushConstants{};
		pushConstants.ModelMatrix = mesh.GetModelMatrix();
		pushConstants.AlbedoTexIndex = mesh.GetMaterialIndices().albedoTexIdx;
		pushConstants.AOTexIndex = mesh.GetMaterialIndices().aoTexIdx;

		vkCmdPushConstants(
			m_CommandBuffers[currentFrame],
			pipeline->GetPipelineLayout(),
			pipeline->GetStageFlags(),
			0,
			sizeof(PushConstants),
			&pushConstants
		);

		VkBuffer vertexBuffers[] = { mesh.GetVertexBuffer() };
		VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(m_CommandBuffers[currentFrame], 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(m_CommandBuffers[currentFrame], mesh.GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(m_CommandBuffers[currentFrame], static_cast<uint32_t>(mesh.GetIndices().size()), 1, 0, 0, 0);
	}
}

VkCommandBuffer CommandManager::BeginSingleTimeCommands(VkDevice device) const
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_CommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void CommandManager::EndSingleTimeCommands(const VkQueue& graphicsQueue, const VkCommandBuffer& commandBuffer , const VkDevice& device) const
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(device, m_CommandPool, 1, &commandBuffer);
}

void CommandManager::TransitionImage(Image& image, TransitionImgContext context,int currentFrame) const
{
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = context.oldLayout;
	barrier.newLayout = context.newLayout;
	image.SetCurrentLayout(context.newLayout);
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image.GetImage();
	barrier.subresourceRange.aspectMask = context.aspectMask;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	barrier.srcAccessMask = context.srcAccessMask;
	barrier.dstAccessMask = context.dstAccessMask;

	VkPipelineStageFlags srcStage = context.srcStage;
	VkPipelineStageFlags dstStage = context.dstStage;

	vkCmdPipelineBarrier(
		m_CommandBuffers[currentFrame],
		srcStage, dstStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);
}

void CommandManager::TransitionImage(VkImage image, TransitionImgContext context, int currentFrame)
{
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = context.oldLayout;
	barrier.newLayout = context.newLayout;
	currentImagesLayouts = context.newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = context.aspectMask;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	barrier.srcAccessMask = context.srcAccessMask;
	barrier.dstAccessMask = context.dstAccessMask;

	VkPipelineStageFlags srcStage = context.srcStage;
	VkPipelineStageFlags dstStage = context.dstStage;

	vkCmdPipelineBarrier(
		m_CommandBuffers[currentFrame],
		srcStage, dstStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);
}

void CommandManager::Destroy(VkDevice device) const
{
	vkDestroyCommandPool(device, m_CommandPool, nullptr);
}
