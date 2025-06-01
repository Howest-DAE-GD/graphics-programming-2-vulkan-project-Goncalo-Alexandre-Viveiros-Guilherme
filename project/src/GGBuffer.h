#pragma once
#include <vulkan/vulkan_core.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include "GGCamera.h"
#include "Scene.h"

struct alignas(16) UniformBufferObject
{
	glm::mat4 sceneMatrix;
	glm::mat4 view;
	glm::mat4 proj;
};

struct alignas(16) LightsUBO {
	Light lights[32];
	glm::vec3 viewPos;
};

namespace GG
{
	class CommandManager;

	class Buffer
	{
	public:
		Buffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, int maxFramesInFlight):
		m_Device(device), m_PhysicalDevice(physicalDevice), m_MaxFramesInFlight(maxFramesInFlight){}

		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
			VkDeviceMemory& bufferMemory) const;

		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkQueue graphicsQueue,  const CommandManager* commandManager) const;

		//---------------------- Uniform Buffer ---------------------------------
		void CreateUniformBuffers();
		void UpdateUniformBuffer(uint32_t currentImage, VkExtent2D swapChainExtent, Scene* scene) const;
		//---------------------- No Uniform Buffer ------------------------------

		void DestroyBuffer() const;

		std::vector<VkBuffer>& GetUniformBuffers() { return uniformBuffers; }
		std::vector<VkBuffer>& GetLightBuffers() { return lightsBuffers; }
	private:


		std::vector<VkBuffer> uniformBuffers;
		std::vector<VkDeviceMemory> uniformBuffersMemory;
		std::vector<void*> uniformBuffersMapped;
		std::vector<VkBuffer> lightsBuffers;
		std::vector<VkDeviceMemory> lightsBuffersMemory;
		std::vector<void*> lightsBuffersMapped;

		const int m_MaxFramesInFlight;

		VkDevice m_Device;
		VkPhysicalDevice m_PhysicalDevice;
	};
}
