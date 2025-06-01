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

void Buffer::CreateUniformBuffers() {
	// Matrix UBO
	VkDeviceSize matrixBufferSize = sizeof(UniformBufferObject);
	uniformBuffers.resize(m_MaxFramesInFlight);
	uniformBuffersMemory.resize(m_MaxFramesInFlight);
	uniformBuffersMapped.resize(m_MaxFramesInFlight);

	// Lights UBO
	VkDeviceSize lightsBufferSize = sizeof(LightsUBO);
	lightsBuffers.resize(m_MaxFramesInFlight);
	lightsBuffersMemory.resize(m_MaxFramesInFlight);
	lightsBuffersMapped.resize(m_MaxFramesInFlight);

	for (size_t i = 0; i < m_MaxFramesInFlight; i++) {
		// Create matrix UBO
		CreateBuffer(
			matrixBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			uniformBuffers[i],
			uniformBuffersMemory[i]
		);
		vkMapMemory(m_Device, uniformBuffersMemory[i], 0, matrixBufferSize, 0, &uniformBuffersMapped[i]);

		// Create lights UBO
		CreateBuffer(
			lightsBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			lightsBuffers[i],
			lightsBuffersMemory[i]
		);
		vkMapMemory(m_Device, lightsBuffersMemory[i], 0, lightsBufferSize, 0, &lightsBuffersMapped[i]);
	}
}

void Buffer::UpdateUniformBuffer(uint32_t currentImage, VkExtent2D swapChainExtent, Scene* scene) const {
	// --- Update Matrices ---
	UniformBufferObject ubo{};
	ubo.view = (scene->GetCamera().GetViewMatrix());
	ubo.proj = scene->GetCamera().GetProjectionMatrix();
	ubo.proj[1][1] *= -1;
	ubo.sceneMatrix = scene->GetSceneMatrix();

	memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
	// --- Update Lights ---
	LightsUBO lightsUbo{};
	const auto& lights = scene->GetLights();

	for (size_t i = 0; i < lights.size(); i++) 
	{  
		lightsUbo.lights[i].Position = lights[i].Position;
		lightsUbo.lights[i].Color = lights[i].Color;
		lightsUbo.lights[i].Radius = lights[i].Radius;
	}

	lightsUbo.viewPos = scene->GetCamera().GetPosition();

	memcpy(lightsBuffersMapped[currentImage], &lightsUbo, sizeof(lightsUbo));
}


//---------------------- No Uniform Buffer ------------------------------


void Buffer::DestroyBuffer() const {
	for (size_t i = 0; i < m_MaxFramesInFlight; i++) {
		vkDestroyBuffer(m_Device, uniformBuffers[i], nullptr);
		vkFreeMemory(m_Device, uniformBuffersMemory[i], nullptr);

		vkDestroyBuffer(m_Device, lightsBuffers[i], nullptr);
		vkFreeMemory(m_Device, lightsBuffersMemory[i], nullptr);
	}
}
