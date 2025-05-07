#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>

namespace GG
{
	class VkErrorHandler;
	class SwapChain;

	class Device
	{
	public:
		Device() = default;
		~Device() = default;

		void InitializeDevice(VkInstance& instance, VkSurfaceKHR& surface, bool isValidationLayerEnabled, VkErrorHandler& errorHandler);
		void CreateLogicalDevice(VkSurfaceKHR& surface, bool isValidationLayerEnabled, VkErrorHandler& errorHandler);
		void PickPhysicalDevice(const VkInstance& instance, VkSurfaceKHR& surface);
		bool IsDeviceSuitable(VkPhysicalDevice& physicalDevice, VkSurfaceKHR& surface) const;
		bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const;

		void DeviceWaitIdle() const;

		void DestroyDevice() const;

		//multisampling
		void GetMaxUsableSampleCount();
		void CreateTextureSampler();

		VkSampleCountFlagBits& GetMssaSamples() { return m_MsaaSamples; }
		VkSampler& GetTextureSampler() { return m_TextureSampler; }

		VkDevice& GetVulkanDevice() {return m_Device;}
		VkPhysicalDevice& GetVulkanPhysicalDevice() {return m_PhysicalDevice;}

		VkQueue& GetGraphicsQueue() { return m_GraphicsQueue; }
		VkQueue& GetPresentQueue() { return m_PresentQueue; }

	private:
		VkDevice m_Device;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;

		VkSampler m_TextureSampler;
		VkSampleCountFlagBits m_MsaaSamples = VK_SAMPLE_COUNT_1_BIT;

		const std::vector<const char*> m_DeviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		VkQueue m_GraphicsQueue;
		VkQueue m_PresentQueue;

	};

}

