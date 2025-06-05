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
		void CreateUniformBuffers(Scene* scene);
		void UpdateUniformBuffer(uint32_t currentImage, VkExtent2D swapChainExtent, Scene* scene) const;
		//---------------------- No Uniform Buffer ------------------------------

		void DestroyBuffer() const;

		std::vector<VkBuffer>& GetUniformBuffers() { return m_UniformBuffers; }
		std::vector<VkBuffer>& GetPointLightBuffers() { return m_PointLightsBuffers; }
		std::vector<VkBuffer>& GetDirLightBuffers() { return m_DirectionalLightsBuffers; }
	private:


		std::vector<VkBuffer> m_UniformBuffers;
		std::vector<VkDeviceMemory> m_UniformBuffersMemory;
		std::vector<void*> m_UniformBuffersMapped;

		std::vector<VkBuffer> m_PointLightsBuffers;
		std::vector<VkDeviceMemory> m_PointLightsBuffersMemory;
		std::vector<void*> m_PointLightsBuffersMapped;

		std::vector<VkBuffer> m_DirectionalLightsBuffers;
		std::vector<VkDeviceMemory> m_DirectionalLightsBuffersMemory;
		std::vector<void*> m_DirectionalLightsBuffersMapped;

		const int m_MaxFramesInFlight;

		VkDevice m_Device;
		VkPhysicalDevice m_PhysicalDevice;
	};
}
