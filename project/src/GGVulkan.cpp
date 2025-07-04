#include "GGVulkan.h"

#include <stb_image.h>
#include <tiny_obj_loader.h>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include "GGSwapChain.h"

#include "GGBuffer.h"
#include "GGDescriptorManager.h"
#include "GGPipeLine.h"
#include "GGVkDevice.h"
#include "GGVkHelperFunctions.h"
#include "Time.h"
#include "GGShader.h"


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
		m_pPrePassPipeline = new GG::Pipeline();
		m_pLightingPipeline = new GG::Pipeline();
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

		m_GBuffer.CreateImages(m_VkSwapChain->GetSwapChainExtent(), m_Device);
		m_BlitPass.CreateImage(m_VkSwapChain->GetSwapChainExtent(), m_Device);

		m_pBuffer = new GG::Buffer(device, physicalDevice,m_MaxFramesInFlight);

		if (m_CurrentScene->GetTextureCount() <= 0)
		{
			throw std::runtime_error("Cannot create descriptor pool with zero textures.");
		}

		CreateDescriptorSetLayout4PrePass();
		m_GBuffer.CreateDescriptorSetLayout(m_Device,m_pDescriptorManager);
		CreateDescriptorSetLayoutLighting();
		m_BlitPass.CreateDescriptorSetLayout(m_Device, m_pDescriptorManager);

		CreateDepthPrePassPipeline();
		m_GBuffer.CreatePipeline(m_Device,m_pDescriptorManager);
		CreateLightingPipeline();
		CreateLightingPipeline();
		m_BlitPass.CreateBlitPipeline(m_Device, m_pDescriptorManager,m_VkSwapChain->GetSwapChainImgFormat());

		m_pCommandManager->CreateCommandPool(device,physicalDevice,m_Surface);
		m_VkSwapChain->CreateColorResources(mssaSamples);
		m_VkSwapChain->CreateDepthResources(mssaSamples);

		m_CurrentScene->CreateImages(m_pBuffer, m_pCommandManager, m_Device->GetGraphicsQueue(), device, physicalDevice);

		m_Device->CreateTextureSampler();

		m_CurrentScene->CreateMeshBuffers(m_Device,m_pBuffer,m_pCommandManager);

		m_pBuffer->CreateUniformBuffers(m_CurrentScene);

		CreateDescriptorPool4PrePass();
		m_GBuffer.CreateDescriptorPool(m_Device,m_pDescriptorManager,m_MaxFramesInFlight);
		CreateDescriptorPoolLighting();
		m_BlitPass.CreateDescriptorPool(m_Device, m_pDescriptorManager, m_MaxFramesInFlight);

		CreateDescriptorSets4PrePass();
		m_GBuffer.CreateDescriptorSets(m_CurrentScene,m_Device,m_pDescriptorManager,m_pBuffer,m_MaxFramesInFlight);
		CreateDescriptorSetsLighting();
		m_BlitPass.CreateDescriptorSets(m_Device, m_pDescriptorManager, m_MaxFramesInFlight);

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

		GG::PipelinesForCommandBuffer pipelinesForCommandBuffer{ m_pPrePassPipeline,m_GBuffer.GetPipeline(),
			m_pLightingPipeline,m_BlitPass.GetPipeline()};

		m_pCommandManager->RecordCommandBuffer(imageIndex,m_VkSwapChain, m_CurrentFrame, m_GBuffer , m_BlitPass,
			pipelinesForCommandBuffer,m_CurrentScene,m_pDescriptorManager);


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

	void GGVulkan::CreateDescriptorSets4PrePass() const
	{
		DescriptorSetsContext descriptorSetsContext;

		descriptorSetsContext.VariableCount = static_cast<uint32_t>(m_CurrentScene->GetImageViews().size());

		descriptorSetsContext.BufferInfos.resize(m_MaxFramesInFlight);

		for (size_t i = 0; i < m_MaxFramesInFlight; i++)
		{
			auto& bufferInfo = descriptorSetsContext.BufferInfos[i];
			bufferInfo.buffer = m_pBuffer->GetUniformBuffers()[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkWriteDescriptorSet uniformBufferDescriptor{};
			uniformBufferDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			uniformBufferDescriptor.pNext = nullptr;
			uniformBufferDescriptor.dstSet = VK_NULL_HANDLE;  // will be overwritten in Manager
			uniformBufferDescriptor.dstBinding = 0;
			uniformBufferDescriptor.dstArrayElement = 0;
			uniformBufferDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uniformBufferDescriptor.descriptorCount = 1;
			uniformBufferDescriptor.pBufferInfo = &descriptorSetsContext.BufferInfos[i];

			descriptorSetsContext.AddDescriptorSetWrites(std::move(uniformBufferDescriptor));
		}

		// 1) build SetLayouts array � one layout handle per frame
		descriptorSetsContext.SetLayouts.assign(
			m_MaxFramesInFlight,
			m_pDescriptorManager->GetDescriptorSetLayout(0)  // same layout for each frame
		);

		// 2) zero-init and fill AllocateInfo
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;
		allocInfo.descriptorPool = m_pDescriptorManager->GetDescriptorPool(0);
		allocInfo.descriptorSetCount = static_cast<uint32_t>(descriptorSetsContext.SetLayouts.size());
		allocInfo.pSetLayouts = descriptorSetsContext.SetLayouts.data();

		descriptorSetsContext.AllocateInfo = allocInfo;
		descriptorSetsContext.DescriptorSetLayout = m_pDescriptorManager->GetDescriptorSetLayout(0);

		m_pDescriptorManager->CreateDescriptorSets(std::move(descriptorSetsContext), m_MaxFramesInFlight, m_Device->GetVulkanDevice());
	}

	void GGVulkan::CreateDescriptorSetsLighting()
	{
		DescriptorSetsContext descriptorSetsContext;

		// [1] Prepare image infos (same for all frames)
		std::vector<VkDescriptorImageInfo> imageInfos(4); // Albedo, Normal, MR, Depth

		// Albedo + AO (binding 0)
		imageInfos[0] = {
			.sampler = m_Device->GetTextureSampler(),
			.imageView = m_GBuffer.GetAlbedoGGImage().GetImageView(),
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		// Normal (binding 1)
		imageInfos[1] = {
			.sampler = m_Device->GetTextureSampler(),
			.imageView = m_GBuffer.GetNormalMapGGImage().GetImageView(),
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		// Metallic-Roughness (binding 2)
		imageInfos[2] = {
			.sampler = m_Device->GetTextureSampler(),
			.imageView = m_GBuffer.GetMettalicRoughnessGGImage().GetImageView(),
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		// Depth (binding 4)
		imageInfos[3] = {
			.sampler = m_Device->GetTextureSampler(),
			.imageView = m_VkSwapChain->GetDepthImageView(),
			.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
		};

		// [2] Prepare writes for each frame
		for (size_t i = 0; i < m_MaxFramesInFlight; ++i) {
			// Point Lights Ssbo (binding 3)
			VkDescriptorBufferInfo pointLightsBufferInfo = {
				.buffer = m_pBuffer->GetPointLightBuffers()[i],
				.offset = 0,
				.range = sizeof(PointLight) * m_CurrentScene->GetPointLights().size()
			};

			VkDescriptorBufferInfo dirLightsBufferInfo = {
				.buffer = m_pBuffer->GetDirLightBuffers()[i],
				.offset = 0,
				.range = sizeof(DirectionalLight) * m_CurrentScene->GetDirectionalLights().size()
			};

			// Camera UBO (binding 5)
			VkDescriptorBufferInfo cameraBufferInfo = {            //todo change this to be just a invViewMatrix maybe
				.buffer = m_pBuffer->GetUniformBuffers()[i],
				.offset = 0,
				.range = sizeof(UniformBufferObject) 
			};

			// [3] Create descriptor writes
			// Albedo (binding 0)
			VkWriteDescriptorSet albedoWrite = {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstBinding = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = &imageInfos[0]
			};

			// Normal (binding 1)
			VkWriteDescriptorSet normalWrite = {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstBinding = 1,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = &imageInfos[1]
			};

			// Metallic-Roughness (binding 2)
			VkWriteDescriptorSet metallicRoughnessWrite = {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstBinding = 2,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = &imageInfos[2]
			};

			// Point Lights SSBO (binding 3)
			VkWriteDescriptorSet pointLightsWrite = {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstBinding = 3,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
				.pBufferInfo = &pointLightsBufferInfo
			};

			// Direcitional Lights SSBO (binding 6)
			VkWriteDescriptorSet dirLightsWrite = {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstBinding = 6,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
				.pBufferInfo = &dirLightsBufferInfo
			};

			// Depth (binding 4)
			VkWriteDescriptorSet depthWrite = {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstBinding = 4,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = &imageInfos[3]
			};

			// Camera UBO (binding 5)
			VkWriteDescriptorSet cameraWrite = {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstBinding = 5,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.pBufferInfo = &cameraBufferInfo
			};

			descriptorSetsContext.AddDescriptorSetWrites(albedoWrite);
			descriptorSetsContext.AddDescriptorSetWrites(normalWrite);
			descriptorSetsContext.AddDescriptorSetWrites(metallicRoughnessWrite);
			descriptorSetsContext.AddDescriptorSetWrites(pointLightsWrite);
			descriptorSetsContext.AddDescriptorSetWrites(dirLightsWrite);
			descriptorSetsContext.AddDescriptorSetWrites(depthWrite);
			descriptorSetsContext.AddDescriptorSetWrites(cameraWrite);
		}

		// [4] Set up allocation info
		descriptorSetsContext.SetLayouts.assign(
			m_MaxFramesInFlight,
			m_pDescriptorManager->GetDescriptorSetLayout(2)
		);

		VkDescriptorSetAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = m_pDescriptorManager->GetDescriptorPool(2),
			.descriptorSetCount = static_cast<uint32_t>(m_MaxFramesInFlight),
			.pSetLayouts = descriptorSetsContext.SetLayouts.data()
		};

		descriptorSetsContext.AllocateInfo = allocInfo;
		descriptorSetsContext.DescriptorSetLayout = m_pDescriptorManager->GetDescriptorSetLayout(2);

		m_pDescriptorManager->CreateDescriptorSets(std::move(descriptorSetsContext), m_MaxFramesInFlight, m_Device->GetVulkanDevice());
	}


	void GGVulkan::CreateDescriptorPool4PrePass() const
	{
		VkDescriptorPoolSize poolSize;
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = static_cast<uint32_t>(m_MaxFramesInFlight);
	
		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = static_cast<uint32_t>(m_MaxFramesInFlight);
	
		DescriptorPoolContext poolContext;
		poolContext.DescriptorPoolInfo = poolInfo;
		poolContext.AddPoolSize(poolSize);
	
		m_pDescriptorManager->CreateDescriptorPool(m_Device->GetVulkanDevice(), m_MaxFramesInFlight, std::move(poolContext));
	}

	void GGVulkan::CreateDescriptorPoolLighting() const
	{
		std::vector<VkDescriptorPoolSize> poolSizes(3);
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[0].descriptorCount = 3 * m_MaxFramesInFlight;

		poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[1].descriptorCount = 1 * m_MaxFramesInFlight;

		poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC; 
		poolSizes[2].descriptorCount = 2 * m_MaxFramesInFlight;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = m_MaxFramesInFlight;

		DescriptorPoolContext poolContext;
		poolContext.DescriptorPoolInfo = poolInfo;
		for (const auto& size : poolSizes)
			poolContext.AddPoolSize(size);

		m_pDescriptorManager->CreateDescriptorPool(m_Device->GetVulkanDevice(), m_MaxFramesInFlight, std::move(poolContext));
	}


	void GGVulkan::CreateDescriptorSetLayout4PrePass() const
	{
		DescriptorSetLayoutContext descriptorSetLayoutContext;
	
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.pImmutableSamplers = nullptr;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	
		descriptorSetLayoutContext.AddDescriptorSetLayout(uboLayoutBinding);
		descriptorSetLayoutContext.BindingFlags.emplace_back(0);
		descriptorSetLayoutContext.DescriptorSetLayoutIndex = 0;
	
		m_pDescriptorManager->CreateDescriptorSetLayout(m_Device->GetVulkanDevice(), std::move(descriptorSetLayoutContext));
	}
	
	void GGVulkan::CreateDescriptorSetLayoutLighting() const
	{
		DescriptorSetLayoutContext descriptorSetLayoutContext;

		// Combined image samplers
		VkDescriptorSetLayoutBinding albedoBinding = {
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
		};

		VkDescriptorSetLayoutBinding normalBinding = {
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
		};

		VkDescriptorSetLayoutBinding metallicRoughnessBinding = {
			.binding = 2,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
		};

		// Uniform buffers
		VkDescriptorSetLayoutBinding pointLightsBinding = {
			.binding = 3,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
		};

		VkDescriptorSetLayoutBinding depthBinding = {
			.binding = 4,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
		};

		VkDescriptorSetLayoutBinding cameraBinding = {
			.binding = 5,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
		};

		VkDescriptorSetLayoutBinding dirLightsBinding = {
			.binding = 6,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
		};

		descriptorSetLayoutContext.AddDescriptorSetLayout(albedoBinding);
		descriptorSetLayoutContext.AddDescriptorSetLayout(normalBinding);
		descriptorSetLayoutContext.AddDescriptorSetLayout(metallicRoughnessBinding);
		descriptorSetLayoutContext.AddDescriptorSetLayout(pointLightsBinding);
		descriptorSetLayoutContext.AddDescriptorSetLayout(depthBinding);
		descriptorSetLayoutContext.AddDescriptorSetLayout(cameraBinding);
		descriptorSetLayoutContext.AddDescriptorSetLayout(dirLightsBinding);

		descriptorSetLayoutContext.DescriptorSetLayoutIndex = 2;
		descriptorSetLayoutContext.BindingFlags = { 0, 0, 0, 0, 0, 0, 0}; // No special flags needed

		m_pDescriptorManager->CreateDescriptorSetLayout(m_Device->GetVulkanDevice(), std::move(descriptorSetLayoutContext));
	}


	void GGVulkan::CreateDepthPrePassPipeline() const
	{
		PipelineContext depthPrePassPipeline{};

		GG::Shader vertShader{ "shaders/depthPrePassShader.vert.spv" , m_Device->GetVulkanDevice() };

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShader.GetShaderModule();
		vertShaderStageInfo.pName = "main";

		depthPrePassPipeline.ShaderStages = { vertShaderStageInfo };

		VkVertexInputAttributeDescription attributeDescription{};
		attributeDescription.binding = 0;
		attributeDescription.location = 0;
		attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescription.offset = offsetof(Vertex, pos);

		depthPrePassPipeline.AttributeDescriptions.clear();
		depthPrePassPipeline.AttributeDescriptions.emplace_back(attributeDescription);

		depthPrePassPipeline.VertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		depthPrePassPipeline.VertexInputState.pNext = nullptr;
		depthPrePassPipeline.VertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(depthPrePassPipeline.AttributeDescriptions.size());
		depthPrePassPipeline.VertexInputState.pVertexAttributeDescriptions = depthPrePassPipeline.AttributeDescriptions.data();
		depthPrePassPipeline.VertexInputState.vertexBindingDescriptionCount = 1;


		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = 0;
		colorBlendAttachment.blendEnable = VK_FALSE;

		depthPrePassPipeline.ColorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		depthPrePassPipeline.ColorBlendState.logicOpEnable = VK_FALSE;
		depthPrePassPipeline.ColorBlendState.logicOp = VK_LOGIC_OP_COPY;
		depthPrePassPipeline.ColorBlendState.attachmentCount = 1;
		depthPrePassPipeline.ColorBlendState.pAttachments = &colorBlendAttachment;

		depthPrePassPipeline.DepthStencilState.depthWriteEnable = VK_TRUE;
		depthPrePassPipeline.DepthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;

		depthPrePassPipeline.MultisampleState.rasterizationSamples = m_Device->GetMssaSamples();

		VkShaderStageFlags pipelineStageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		depthPrePassPipeline.PushConstantRange.stageFlags = pipelineStageFlags;

		depthPrePassPipeline.DepthAttachmentFormat = GG::VkHelperFunctions::FindDepthFormat(m_Device->GetVulkanPhysicalDevice());

		m_pPrePassPipeline->CreatePipeline(m_Device->GetVulkanDevice(), m_pDescriptorManager->GetDescriptorSetLayout(0), depthPrePassPipeline);
	}

	void GGVulkan::CreateLightingPipeline() const
	{
		PipelineContext LightingPipelineContext{};

		GG::Shader vertShader{ "shaders/lightShader.vert.spv" , m_Device->GetVulkanDevice() };
		GG::Shader fragShader{ "shaders/lightShader.frag.spv" , m_Device->GetVulkanDevice() };

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShader.GetShaderModule();
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShader.GetShaderModule();
		fragShaderStageInfo.pName = "main";

		LightingPipelineContext.ShaderStages = { vertShaderStageInfo, fragShaderStageInfo };

		LightingPipelineContext.VertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		LightingPipelineContext.VertexInputState.vertexBindingDescriptionCount = 0;
		LightingPipelineContext.VertexInputState.pVertexBindingDescriptions = nullptr;
		LightingPipelineContext.VertexInputState.vertexAttributeDescriptionCount = 0;
		LightingPipelineContext.VertexInputState.pVertexAttributeDescriptions = nullptr;

		LightingPipelineContext.InputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		LightingPipelineContext.InputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		LightingPipelineContext.InputAssemblyState.primitiveRestartEnable = VK_FALSE;

		LightingPipelineContext.RasterizerState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		LightingPipelineContext.RasterizerState.depthClampEnable = VK_FALSE;
		LightingPipelineContext.RasterizerState.rasterizerDiscardEnable = VK_FALSE;
		LightingPipelineContext.RasterizerState.polygonMode = VK_POLYGON_MODE_FILL;
		LightingPipelineContext.RasterizerState.cullMode = VK_CULL_MODE_NONE; 
		LightingPipelineContext.RasterizerState.frontFace = VK_FRONT_FACE_CLOCKWISE;
		LightingPipelineContext.RasterizerState.lineWidth = 1.0f;

		LightingPipelineContext.ColorAttachmentFormats.emplace_back(VK_FORMAT_R16G16B16A16_SFLOAT); 

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;

		LightingPipelineContext.ColorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		LightingPipelineContext.ColorBlendState.logicOpEnable = VK_FALSE;
		LightingPipelineContext.ColorBlendState.attachmentCount = 1;
		LightingPipelineContext.ColorBlendState.pAttachments = &colorBlendAttachment;

		LightingPipelineContext.DepthStencilState.depthTestEnable = VK_FALSE;
		LightingPipelineContext.DepthStencilState.depthWriteEnable = VK_FALSE;
		LightingPipelineContext.DepthStencilState.stencilTestEnable = VK_FALSE;

		LightingPipelineContext.DepthAttachmentFormat = GG::VkHelperFunctions::FindDepthFormat(m_Device->GetVulkanPhysicalDevice());

		LightingPipelineContext.MultisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		LightingPipelineContext.MultisampleState.rasterizationSamples = m_Device->GetMssaSamples();
		LightingPipelineContext.MultisampleState.sampleShadingEnable = VK_FALSE;

		m_pLightingPipeline->CreatePipeline(m_Device->GetVulkanDevice(), m_pDescriptorManager->GetDescriptorSetLayout(2), LightingPipelineContext);
	}


	bool GGVulkan::HasStencilComponent(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	void GGVulkan::Cleanup() const
	{
		const auto& device = m_Device->GetVulkanDevice();

		m_VkSwapChain->CleanupSwapChain();

		m_BlitPass.Cleanup(device);

		m_GBuffer.CleanUp(device);

		m_pBuffer->DestroyBuffer();

		m_pDescriptorManager->Destroy(device);

		m_CurrentScene->Destroy(device);

		m_BlitPass.DestroyPipeline(device);

		m_pLightingPipeline->Destroy(device);

		m_GBuffer.DestroyPipeline(device);

		m_pPrePassPipeline->Destroy(device);

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