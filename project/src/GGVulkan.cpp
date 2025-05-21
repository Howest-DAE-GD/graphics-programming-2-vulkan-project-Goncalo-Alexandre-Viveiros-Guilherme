#define STB_IMAGE_IMPLEMENTATION
#include "GGVulkan.h"

#include <stb_image.h>
#include <tiny_obj_loader.h>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include "GGSwapChain.h"
#include "Scene.h"

#include "GGBuffer.h"
#include "GGDescriptorManager.h"
#include "GGPipeLine.h"
#include "GGVkDevice.h"
#include "GGVkHelperFunctions.h"
#include "Time.h"


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
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		m_Window = glfwCreateWindow(m_Width, m_Height, "Vulkan", nullptr, nullptr);
		glfwSetWindowUserPointer(m_Window, this);
		glfwSetFramebufferSizeCallback(m_Window, FramebufferResizeCallback);
	}

	void GGVulkan::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		auto app = reinterpret_cast<GGVulkan*>(glfwGetWindowUserPointer(window));
		app->m_FramebufferResized = true;
	}

	void GGVulkan::InitVulkan()
	{
		m_pCommandManager = new GG::CommandManager();
		m_pDescriptorManager = new GG::DescriptorManager();
		m_pPipeline = new GG::Pipeline();
		m_Device = new GG::Device{};

		m_CurrentScene = m_Scenes[0];
		m_CurrentScene->Initialize(m_Window);


		auto& device = m_Device->GetVulkanDevice();
		const auto& physicalDevice = m_Device->GetVulkanPhysicalDevice();
		auto& mssaSamples = m_Device->GetMssaSamples();

		CreateInstance();
		SetupDebugMessenger();
		CreateSurface();

		m_Device->InitializeDevice(m_Instance, m_Surface, m_EnableValidationLayers, m_ErrorHandler);

		m_VkSwapChain = new GG::SwapChain{device,physicalDevice};

		m_VkSwapChain->CreateSwapChain(m_Surface,m_Window);
		m_VkSwapChain->CreateImageViews();
		CreateRenderPass();

		m_pBuffer = new GG::Buffer(device, physicalDevice,m_MaxFramesInFlight);

		if (m_CurrentScene->GetTextureCount() <= 0)
		{
			throw std::runtime_error("Cannot create descriptor pool with zero textures.");
		}
		m_pDescriptorManager->CreateDescriptorSetLayout(device, m_CurrentScene->GetTextureCount());
		m_pPipeline->CreateGraphicsPipeline(device, physicalDevice ,mssaSamples, m_pDescriptorManager->GetDescriptorSetLayout()[0], m_VkSwapChain, m_CurrentScene);
		m_pCommandManager->CreateCommandPool(device,physicalDevice,m_Surface);
		m_VkSwapChain->CreateColorResources(mssaSamples);
		m_VkSwapChain->CreateDepthResources(mssaSamples);

		m_CurrentScene->CreateImages(m_pBuffer, m_pCommandManager, m_Device->GetGraphicsQueue(), device, physicalDevice);

		m_Device->CreateTextureSampler();

		m_CurrentScene->CreateMeshBuffers(m_Device,m_pBuffer,m_pCommandManager);

		m_pBuffer->CreateUniformBuffers();

		m_pDescriptorManager->CreateDescriptorPool(device,m_MaxFramesInFlight, m_CurrentScene->GetTextureCount());
		m_pDescriptorManager->CreateDescriptorSets(m_CurrentScene->GetImageViews(), m_Device->GetTextureSampler(),m_MaxFramesInFlight,device, m_pBuffer->GetUniformBuffers());

		m_pCommandManager->CreateCommandBuffers(device,m_MaxFramesInFlight);
		CreateSyncObjects();
	}

	void GGVulkan::CreateSurface()
	{
		if (glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &m_Surface) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create window surface!");
		}
	}

	void GGVulkan::MainLoop()
	{
		while (!glfwWindowShouldClose(m_Window))
		{
			Time::Update();
			m_CurrentScene->Update();
			glfwPollEvents();

			//if (glfwGetKey(m_Window, GLFW_KEY_F2) == GLFW_PRESS) m_CurrentScene = m_Scenes [1]; TODO: Create a proper scene switching system
			DrawFrame();
		}
		m_Device->DeviceWaitIdle();
	}

	void GGVulkan::DrawFrame()
	{
		vkWaitForFences(m_Device->GetVulkanDevice(), 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(m_Device->GetVulkanDevice(), m_VkSwapChain->GetSwapChain(), 
			UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FramebufferResized)
		{
			VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };

			VkSubmitInfo submitInfo{
				.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
					.waitSemaphoreCount = 1,
					.pWaitSemaphores = &m_ImageAvailableSemaphores[m_CurrentFrame],
					.pWaitDstStageMask = waitStages,
					.commandBufferCount = 0,
					.signalSemaphoreCount = 0
			};

			vkQueueSubmit(m_Device->GetGraphicsQueue(),1, &submitInfo,VK_NULL_HANDLE);
			vkDeviceWaitIdle(m_Device->GetVulkanDevice());
			m_FramebufferResized = false;
			m_VkSwapChain->RecreateSwapChain(m_Device->GetMssaSamples(),m_Window,m_RenderPass,m_Surface);
			return;
		}
		else if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		m_pBuffer->UpdateUniformBuffer(m_CurrentFrame,m_VkSwapChain->GetSwapChainExtent(), m_CurrentScene);

		vkResetFences(m_Device->GetVulkanDevice(), 1, &m_InFlightFences[m_CurrentFrame]);

		vkResetCommandBuffer(m_pCommandManager->GetCommandBuffers()[m_CurrentFrame], /*VkCommandBufferResetFlagBits*/ 0);
		m_pCommandManager->RecordCommandBuffer(imageIndex,m_VkSwapChain, m_CurrentFrame,m_pPipeline,m_CurrentScene,m_pDescriptorManager->GetDescriptorSets());


		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		const VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
		constexpr VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_pCommandManager->GetCommandBuffers()[m_CurrentFrame];

		const VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &submitInfo, m_InFlightFences[m_CurrentFrame]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		const VkSwapchainKHR swapChains[] = { m_VkSwapChain->GetSwapChain() };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;

		presentInfo.pImageIndices = &imageIndex;

		result = vkQueuePresentKHR(m_Device->GetPresentQueue(), &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
			m_VkSwapChain->RecreateSwapChain(m_Device->GetMssaSamples(),m_Window,m_RenderPass,m_Surface);
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}

		m_CurrentFrame = (m_CurrentFrame + 1) % m_MaxFramesInFlight;
	}

	void GGVulkan::SetupDebugMessenger() {
		if (!m_EnableValidationLayers) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		m_ErrorHandler.PopulateDebugMessengerCreateInfo(createInfo);

		if (m_ErrorHandler.CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}

	std::vector<const char*> GGVulkan::GetRequiredExtensions() const
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (m_EnableValidationLayers)
	{
		m_ErrorHandler.GetRequiredExtensions(extensions);
	}

	return extensions;
}
	
	void GGVulkan::CreateInstance()
	{
		if (m_EnableValidationLayers && !m_ErrorHandler.CheckValidationLayerSupport())
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
		appInfo.apiVersion = VK_API_VERSION_1_3;

		//Create Info
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		auto extensions = GetRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		//validation layer code for create info

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (m_EnableValidationLayers)
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

		if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}

	}

	//---------------------- Render Pass ---------------------------------------

	void GGVulkan::CreateRenderPass()
	{
		const auto& swapChainImageFormat = m_VkSwapChain->GetSwapChainImgFormat();
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapChainImageFormat;
		colorAttachment.samples = m_Device->GetMssaSamples();
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
		depthAttachment.format = GG::VkHelperFunctions::FindDepthFormat(m_Device->GetVulkanPhysicalDevice());
		depthAttachment.samples = m_Device->GetMssaSamples();
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

		if (vkCreateRenderPass(m_Device->GetVulkanDevice(), &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}
	}

	//---------------------- No longerRender Pass ------------------------------
	//---------------------- Sync objcs ----------------------------------
	void GGVulkan::CreateSyncObjects()
	{
		const auto& device = m_Device->GetVulkanDevice();

		m_ImageAvailableSemaphores.resize(m_MaxFramesInFlight);
		m_RenderFinishedSemaphores.resize(m_MaxFramesInFlight);
		m_InFlightFences.resize(m_MaxFramesInFlight);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		for (int i = 0; i < m_MaxFramesInFlight; i++)
		{
			if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(device, &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create synchronization objects for a frame!");
			}
		}

	}
	//---------------------- No Sync objcs ----------------------------------

	bool GGVulkan::HasStencilComponent(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	void GGVulkan::Cleanup() const
	{
		const auto& device = m_Device->GetVulkanDevice();

		m_VkSwapChain->CleanupSwapChain();

		m_pBuffer->DestroyBuffer();

		m_pDescriptorManager->Destroy(device);

		m_CurrentScene->Destroy(device);

		m_pPipeline->Destroy(device);

		vkDestroyRenderPass(device, m_RenderPass, nullptr);

		for (size_t i = 0; i < m_MaxFramesInFlight; i++)
		{
			vkWaitForFences(device, 1, &m_InFlightFences[i], VK_TRUE, UINT64_MAX);
			vkDestroySemaphore(device, m_RenderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(device, m_ImageAvailableSemaphores[i], nullptr);
			vkDestroyFence(device, m_InFlightFences[i], nullptr);
		}

		m_pCommandManager->Destroy(device);

		if (m_EnableValidationLayers)
		{
			m_ErrorHandler.DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
		}

		m_Device->DestroyDevice();
		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
		vkDestroyInstance(m_Instance, nullptr);

		glfwDestroyWindow(m_Window);

		glfwTerminate();

		//Added after tutorial
		delete m_CurrentScene;
		delete m_VkSwapChain;
	}

	void GGVulkan::AddScene(Scene* sceneToAdd)
	{
		m_Scenes.emplace_back(sceneToAdd);
	}