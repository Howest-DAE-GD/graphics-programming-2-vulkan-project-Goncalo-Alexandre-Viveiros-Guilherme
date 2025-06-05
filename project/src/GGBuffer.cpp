#include "GGBuffer.h"

#include <array>
#include <chrono>
#include <stdexcept>

#include "GGCamera.h"
#include "GGCommandManager.h"
#include "GGVkHelperFunctions.h"
#include "Scene.h"
#include "Time.h"

using namespace GG;
void Buffer::CreateBuffer(const VkDeviceSize size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(m_Device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(m_Device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = GG::VkHelperFunctions::FindMemoryType(memRequirements.memoryTypeBits, properties, m_PhysicalDevice);

	if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(m_Device, buffer, bufferMemory, 0);
}

void Buffer::CopyBuffer(const VkBuffer srcBuffer, const VkBuffer dstBuffer, const VkDeviceSize size, const VkQueue graphicsQueue,const CommandManager* commandManager) const
{
	VkCommandBuffer commandBuffer = commandManager->BeginSingleTimeCommands(m_Device);

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	commandManager->EndSingleTimeCommands(graphicsQueue, commandBuffer,m_Device);
}

//---------------------- Uniform Buffer ---------------------------------

void Buffer::CreateUniformBuffers(Scene* scene) {
	// Matrix UBO
	VkDeviceSize matrixBufferSize = sizeof(UniformBufferObject);
	m_UniformBuffers.resize(m_MaxFramesInFlight);
	m_UniformBuffersMemory.resize(m_MaxFramesInFlight);
	m_UniformBuffersMapped.resize(m_MaxFramesInFlight);

	// PointLights SSBO
	VkDeviceSize pointLightsBufferSize = sizeof(PointLight) * scene->GetPointLights().size();
	m_PointLightsBuffers.resize(m_MaxFramesInFlight);
	m_PointLightsBuffersMemory.resize(m_MaxFramesInFlight);
	m_PointLightsBuffersMapped.resize(m_MaxFramesInFlight);

	// PointLights SSBO
	VkDeviceSize dirLightsBufferSize = sizeof(DirectionalLight) * scene->GetDirectionalLights().size();
	m_DirectionalLightsBuffers.resize(m_MaxFramesInFlight);
	m_DirectionalLightsBuffersMemory.resize(m_MaxFramesInFlight);
	m_DirectionalLightsBuffersMapped.resize(m_MaxFramesInFlight);

	for (size_t i = 0; i < m_MaxFramesInFlight; i++) {
		// Create matrix UBO
		CreateBuffer(
			matrixBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_UniformBuffers[i],
			m_UniformBuffersMemory[i]
		);
		vkMapMemory(m_Device, m_UniformBuffersMemory[i], 0, matrixBufferSize, 0, &m_UniformBuffersMapped[i]);

		// Create Point lights Ssbo
		CreateBuffer(
			pointLightsBufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_PointLightsBuffers[i],
			m_PointLightsBuffersMemory[i]
		);
		vkMapMemory(m_Device, m_PointLightsBuffersMemory[i], 0, pointLightsBufferSize, 0, &m_PointLightsBuffersMapped[i]);

		// Create Directional lights Ssbo
		CreateBuffer(
			dirLightsBufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_DirectionalLightsBuffers[i],
			m_DirectionalLightsBuffersMemory[i]
		);
		vkMapMemory(m_Device, m_DirectionalLightsBuffersMemory[i], 0, dirLightsBufferSize, 0, &m_DirectionalLightsBuffersMapped[i]);
	}
}

void Buffer::UpdateUniformBuffer(uint32_t currentImage, VkExtent2D swapChainExtent, Scene* scene) const {
	// --- Update Matrices ---
	UniformBufferObject ubo{};
	ubo.view = (scene->GetCamera().GetViewMatrix());
	ubo.proj = scene->GetCamera().GetProjectionMatrix();
	ubo.proj[1][1] *= -1;
	ubo.sceneMatrix = scene->GetSceneMatrix();
	ubo.viewPos = scene->GetCamera().GetPosition(); 

	memcpy(m_UniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
	// --- Update Lights ---
	auto pointLights = scene->GetPointLights();

	memcpy(m_PointLightsBuffersMapped[currentImage], pointLights.data(), pointLights.size() * sizeof(PointLight));

	auto dirLights = scene->GetDirectionalLights();

	memcpy(m_DirectionalLightsBuffersMapped[currentImage], dirLights.data(), dirLights.size() * sizeof(DirectionalLight));
}


//---------------------- No Uniform Buffer ------------------------------


void Buffer::DestroyBuffer() const {
	for (size_t i = 0; i < m_MaxFramesInFlight; i++) {
		vkDestroyBuffer(m_Device, m_UniformBuffers[i], nullptr);
		vkFreeMemory(m_Device, m_UniformBuffersMemory[i], nullptr);

		vkDestroyBuffer(m_Device, m_PointLightsBuffers[i], nullptr);
		vkFreeMemory(m_Device, m_PointLightsBuffersMemory[i], nullptr);

		vkDestroyBuffer(m_Device, m_DirectionalLightsBuffers[i], nullptr);
		vkFreeMemory(m_Device, m_DirectionalLightsBuffersMemory[i], nullptr);
	}
}
