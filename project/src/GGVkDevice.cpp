#include "GGVkDevice.h"

#include <set>
#include <stdexcept>
#include <string>

#include "GGSwapChain.h"
#include "GGVkHelperFunctions.h"


using namespace GG;


void Device::InitializeDevice(VkInstance& instance, VkSurfaceKHR& surface, bool isValidationLayerEnabled, VkErrorHandler& errorHandler)
{
	PickPhysicalDevice(instance, surface);
	CreateLogicalDevice(surface, isValidationLayerEnabled,errorHandler);
}

//---------------Logical Device Setup------------------------
void Device::CreateLogicalDevice(VkSurfaceKHR& surface,bool isValidationLayerEnabled, VkErrorHandler& errorHandler)
{
	QueueFamilyIndices indices = GG::VkHelperFunctions::FindQueueFamilies(m_PhysicalDevice, surface);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}
	VkPhysicalDeviceVulkan13Features features13 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES, .pNext = nullptr};
	features13.dynamicRendering = VK_TRUE;

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.sampleRateShading = VK_TRUE;

	VkPhysicalDeviceFeatures2 deviceFeatures2{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,.pNext = &features13, .features = deviceFeatures};

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pNext = &deviceFeatures2;
	createInfo.pEnabledFeatures = nullptr;

	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	createInfo.enabledExtensionCount = static_cast<uint32_t>(m_DeviceExtensions.size());
	createInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();

	if (isValidationLayerEnabled)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(errorHandler.GetValidationLayers().size());
		createInfo.ppEnabledLayerNames = errorHandler.GetValidationLayers().data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}



	if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(m_Device, indices.graphicsFamily.value(), 0, &m_GraphicsQueue);
	vkGetDeviceQueue(m_Device, indices.presentFamily.value(), 0, &m_PresentQueue);

}
//---------------no more Logical Device Setup------------------------

//---------------Physical Card Setup stuff--------------------
void Device::PickPhysicalDevice(const VkInstance& instance, VkSurfaceKHR& surface)
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0)
	{
		throw std::runtime_error("Failed to find any GPUS with vulkan support!  :(");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for (auto& device : devices)
	{
		if (IsDeviceSuitable(device, surface))
		{
			m_PhysicalDevice = device;
			GetMaxUsableSampleCount();
			break;
		}
	}

	if (m_PhysicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}


bool Device::IsDeviceSuitable(VkPhysicalDevice& physicalDevice,VkSurfaceKHR& surface) const
{
	QueueFamilyIndices indices = GG::VkHelperFunctions::FindQueueFamilies(physicalDevice, surface);

	const bool extensionsSupported = CheckDeviceExtensionSupport(physicalDevice);

	bool swapChainAdequate = false;

	if (extensionsSupported)
	{
		SwapChainSupportDetails swapChainSupport = GG::SwapChain::QuerySwapChainSupport(surface, physicalDevice);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

	return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

bool Device::CheckDeviceExtensionSupport(const VkPhysicalDevice device) const
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(m_DeviceExtensions.begin(), m_DeviceExtensions.end());

	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

//--------------No Longer Physical Device stuff------------------

void Device::DeviceWaitIdle() const
{
	vkDeviceWaitIdle(m_Device);
}

void Device::DestroyDevice() const
{
	vkDestroySampler(m_Device, m_TextureSampler, nullptr);

	vkDestroyDevice(m_Device, nullptr);
}

//multisampling
void Device::GetMaxUsableSampleCount()
{
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(m_PhysicalDevice, &physicalDeviceProperties);

	VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
	if (counts & VK_SAMPLE_COUNT_64_BIT) { m_MsaaSamples = VK_SAMPLE_COUNT_64_BIT; return; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { m_MsaaSamples = VK_SAMPLE_COUNT_32_BIT; return; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { m_MsaaSamples = VK_SAMPLE_COUNT_16_BIT; return; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { m_MsaaSamples = VK_SAMPLE_COUNT_8_BIT;  return; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { m_MsaaSamples = VK_SAMPLE_COUNT_4_BIT;  return; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { m_MsaaSamples = VK_SAMPLE_COUNT_2_BIT;  return; }

	m_MsaaSamples = VK_SAMPLE_COUNT_1_BIT;
}

void Device::CreateTextureSampler()
{
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;

	samplerInfo.anisotropyEnable = VK_TRUE;

	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);

	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.minLod = 0.0f; // Optional
	samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
	samplerInfo.mipLodBias = 0.0f; // Optional

	if (vkCreateSampler(m_Device, &samplerInfo, nullptr, &m_TextureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}
}