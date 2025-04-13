#define STB_IMAGE_IMPLEMENTATION
#include "GGVulkan.h"

#include <stb_image.h>
#include <tiny_obj_loader.h>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <cstring>
#include "GGSwapChain.h"
#include <limits>
#include <set>

#include "GGBuffer.h"
#include "GGDescriptorManager.h"
#include "GGPipeLine.h"
#include "GGTexture.h"
#include "GGVkHelperFunctions.h"


void GGVulkan::Run()
	{
		InitWindow();
		InitVulkan();
		MainLoop();
		Cleanup();
	}

	void GGVulkan::InitWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);
	}

	void GGVulkan::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		auto app = reinterpret_cast<GGVulkan*>(glfwGetWindowUserPointer(window));
		app->framebufferResized = true;
	}

	void GGVulkan::InitVulkan()
	{
		m_pCommandManager = new GG::CommandManager();
		m_pDescriptorManager = new GG::DescriptorManager();
		m_pTexture = new GG::Texture("resources/textures/viking_room.png");
		m_pPipeline = new GG::Pipeline();
		CreateInstance();
		SetupDebugMessenger();
		CreateSurface();
		PickPhysicalDevice();
		CreateLogicalDevice();

		m_VkSwapChain = new GG::SwapChain{device,physicalDevice};

		m_VkSwapChain->CreateSwapChain(surface,window);
		m_VkSwapChain->CreateImageViews();
		CreateRenderPass();

		m_pBuffer = new GG::Buffer(device, physicalDevice,MAX_FRAMES_IN_FLIGHT);//TODO remove

		m_pDescriptorManager->CreateDescriptorSetLayout(device);
		m_pPipeline->CreateGraphicsPipeline(device, m_pTexture->GetMssaSamples(), m_pDescriptorManager->GetDescriptorSetLayout(),renderPass);
		m_pCommandManager->CreateCommandPool(device,physicalDevice,surface);
		m_VkSwapChain->CreateColorResources(m_pTexture->GetMssaSamples());
		m_VkSwapChain->CreateDepthResources(m_pTexture->GetMssaSamples());
		m_VkSwapChain->CreateFramebuffers(renderPass);
		m_pTexture->CreateTextureImage(m_pBuffer,m_pCommandManager,graphicsQueue,device,physicalDevice);
		m_pTexture->CreateTextureImageView(device);
		m_pTexture->CreateTextureSampler(device,physicalDevice);
		CreateVertexBuffer();
		CreateIndexBuffer();
		m_pBuffer->CreateUniformBuffers();
		m_pDescriptorManager->CreateDescriptorPool(device,MAX_FRAMES_IN_FLIGHT);
		m_pDescriptorManager->CreateDescriptorSets(m_pTexture->GetImageView(), m_pTexture->GetTextureSampler(),MAX_FRAMES_IN_FLIGHT,device, m_pBuffer->GetUniformBuffers());
		m_pCommandManager->CreateCommandBuffers(device,MAX_FRAMES_IN_FLIGHT);
		CreateSyncObjects();
	}

	void GGVulkan::CreateSurface()
	{
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create window surface!");
		}
	}

	void GGVulkan::MainLoop()
	{
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
			DrawFrame();
		}
		vkDeviceWaitIdle(device);
	}

	void GGVulkan::DrawFrame()
	{
		vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(device, m_VkSwapChain->GetSwapChain(), UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
		{
			framebufferResized = false;
			m_VkSwapChain->RecreateSwapChain(m_pTexture->GetMssaSamples(), window, renderPass, surface);
			return;
		}
		else if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		m_pBuffer->UpdateUniformBuffer(currentFrame,m_VkSwapChain->GetSwapChainExtent());

		vkResetFences(device, 1, &inFlightFences[currentFrame]);


		vkResetCommandBuffer(m_pCommandManager->GetCommandBuffers()[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
		m_pCommandManager->RecordCommandBuffer(imageIndex,m_VkSwapChain,renderPass,currentFrame,m_pPipeline,m_Scene,m_pDescriptorManager->GetDescriptorSets());


		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_pCommandManager->GetCommandBuffers()[currentFrame];

		VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { m_VkSwapChain->GetSwapChain() };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;

		presentInfo.pImageIndices = &imageIndex;

		result = vkQueuePresentKHR(presentQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
			m_VkSwapChain->RecreateSwapChain(m_pTexture->GetMssaSamples(),window,renderPass,surface);
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void GGVulkan::SetupDebugMessenger() {
		if (!enableValidationLayers) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		m_ErrorHandler.PopulateDebugMessengerCreateInfo(createInfo);

		if (m_ErrorHandler.CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}

std::vector<const char*> GGVulkan::GetRequiredExtensions() const
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers)
	{
		m_ErrorHandler.GetRequiredExtensions(extensions);
	}

	return extensions;
}

void GGVulkan::CreateInstance()
	{
		if (enableValidationLayers && !m_ErrorHandler.CheckValidationLayerSupport())
		{
			throw std::runtime_error("validation layers requested, but not available");
		}

		//App Info
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		//Create Info
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		auto extensions = GetRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		//validation layer code for create info

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(m_ErrorHandler.GetValidationLayers().size());
			createInfo.ppEnabledLayerNames = m_ErrorHandler.GetValidationLayers().data();
			 
			m_ErrorHandler.PopulateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else
		{
			createInfo.enabledLayerCount = 0;

			createInfo.pNext = nullptr;
		}

		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}

	}

	//---------------Logical Device Setup------------------------
	void GGVulkan::CreateLogicalDevice()
	{
		QueueFamilyIndices indices = GG::VkHelperFunctions::FindQueueFamilies(physicalDevice,surface);

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

		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;
		deviceFeatures.sampleRateShading = VK_TRUE;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(m_ErrorHandler.GetValidationLayers().size());
			createInfo.ppEnabledLayerNames = m_ErrorHandler.GetValidationLayers().data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}



		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create logical device!");
		}

		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
	}
	//---------------no more Logical Device Setup------------------------

	//---------------Physical Card Setup stuff--------------------
	void GGVulkan::PickPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if (deviceCount == 0)
		{
			throw std::runtime_error("Failed to find any GPUS with vulkan support!  :(");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		for (const auto& device : devices)
		{
			if (IsDeviceSuitable(device))
			{
				physicalDevice = device;
				m_pTexture->GetMaxUsableSampleCount(physicalDevice);
				break;
			}
		}

		if (physicalDevice == VK_NULL_HANDLE)
		{
			throw std::runtime_error("failed to find a suitable GPU!");
		}
	}


	bool GGVulkan::IsDeviceSuitable(VkPhysicalDevice physicalDevice)
	{
		QueueFamilyIndices indices = GG::VkHelperFunctions::FindQueueFamilies(physicalDevice,surface);

		const bool extensionsSupported = CheckDeviceExtensionSupport(physicalDevice);

		bool swapChainAdequate = false;

		if (extensionsSupported)
		{
			SwapChainSupportDetails swapChainSupport = m_VkSwapChain->QuerySwapChainSupport(surface,physicalDevice);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

		return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
	}

	bool GGVulkan::CheckDeviceExtensionSupport(VkPhysicalDevice device) const
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto& extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	//--------------No Longer Physical Device stuff------------------

	//---------------------- Render Pass ---------------------------------------

	void GGVulkan::CreateRenderPass()
	{
		const auto& swapChainImageFormat = m_VkSwapChain->GetSwapChainImgFormat();
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapChainImageFormat;
		colorAttachment.samples = m_pTexture->GetMssaSamples();
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = GG::VkHelperFunctions::FindDepthFormat(physicalDevice);
		depthAttachment.samples = m_pTexture->GetMssaSamples();
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription colorAttachmentResolve{};
		colorAttachmentResolve.format = swapChainImageFormat;
		colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentResolveRef{};
		colorAttachmentResolveRef.attachment = 2;
		colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;
		subpass.pResolveAttachments = &colorAttachmentResolveRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}
	}
	//---------------------- No longerRender Pass ------------------------------
	//---------------------- Sync objcs ----------------------------------
	void GGVulkan::CreateSyncObjects()
	{
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create synchronization objects for a frame!");
			}
		}

	}
	//---------------------- No Sync objcs ----------------------------------

	//---------------------- Vertice -----------------------------------------

	void GGVulkan::CreateVertexBuffer()
	{
		VkDeviceSize bufferSize = sizeof(m_Scene->GetSceneVertices()[0]) * m_Scene->GetSceneVertices().size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		m_pBuffer->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, m_Scene->GetSceneVertices().data(), (size_t)bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		m_pBuffer->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Scene->GetVertexBuffer(),m_Scene->GetVertexBufferMemory());

		m_pBuffer->CopyBuffer(stagingBuffer, m_Scene->GetVertexBuffer(), bufferSize,graphicsQueue,currentFrame,m_pCommandManager);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	void GGVulkan::CreateIndexBuffer()
	{
		VkDeviceSize bufferSize = sizeof(m_Scene->GetSceneIndices()[0]) * m_Scene->GetSceneIndices().size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		m_pBuffer->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, m_Scene->GetSceneIndices().data(), (size_t)bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		m_pBuffer->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Scene->GetIndexBuffer(), m_Scene->GetIndexBufferMemory());

		m_pBuffer->CopyBuffer(stagingBuffer, m_Scene->GetIndexBuffer(), bufferSize, graphicsQueue, currentFrame, m_pCommandManager);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	//---------------------- No Vertice -----------------------------------------

	bool GGVulkan::HasStencilComponent(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	void GGVulkan::Cleanup()
	{
		m_VkSwapChain->CleanupSwapChain();

		m_pTexture->DestroyTexture(device);

		m_pBuffer->DestroyBuffer();

		m_pDescriptorManager->Destroy(device);

		m_Scene->Destroy(device);

		m_pPipeline->Destroy(device);

		vkDestroyRenderPass(device, renderPass, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkWaitForFences(device, 1, &inFlightFences[i], VK_TRUE, UINT64_MAX);
			vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(device, inFlightFences[i], nullptr);
		}

		m_pCommandManager->Destroy(device);

		vkDestroyDevice(device, nullptr);

		if (enableValidationLayers)
		{
			m_ErrorHandler.DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}

		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);

		glfwDestroyWindow(window);

		glfwTerminate();

		//Added after tutorial
		delete m_Scene;
		delete m_VkSwapChain;
	}

	void GGVulkan::AddScene(Scene* sceneToAdd)
	{
		m_Scene = sceneToAdd;
	}