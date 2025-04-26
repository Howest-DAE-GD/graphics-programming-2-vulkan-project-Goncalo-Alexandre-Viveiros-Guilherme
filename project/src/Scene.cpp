#include "Scene.h"

#include <stdexcept>

#include "GGBuffer.h"
#include "GGVkDevice.h"
#include "tiny_obj_loader.h"

void Scene::AddModel(const Model& modelToAdd)
{
	m_Models.push_back(modelToAdd);

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, modelToAdd.GetModelPath().c_str()))
	{
		throw std::runtime_error(warn + err);
	}

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};

	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex{};

			vertex.pos =
			{
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.texCoord =
			{
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.color = { 1.0f, 1.0f, 1.0f };

			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(m_SceneVertices.size());
				m_SceneVertices.push_back(vertex);
			}

			m_SceneIndices.push_back(uniqueVertices[vertex]);
		}
	}
}

void Scene::AddModel(const std::initializer_list<Model>& modelsToAdd)
{
	for (auto& model: modelsToAdd)
	{
		AddModel(model);
	}
}

void Scene::CreateIndexAndVertexBuffer(GG::Device* pDevice, const GG::Buffer* pBuffer,const GG::CommandManager* pCommandManager)
{
	CreateVertexBuffer(pDevice, pBuffer, pCommandManager);
	CreateIndexBuffer(pDevice, pBuffer, pCommandManager);
}

void Scene::CreateVertexBuffer(GG::Device* pDevice, const GG::Buffer* pBuffer, const GG::CommandManager* pCommandManager)
{
	const auto& device = pDevice->GetVulkanDevice();

	VkDeviceSize bufferSize = sizeof(m_SceneVertices[0]) * m_SceneVertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	pBuffer->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, m_SceneVertices.data(), (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	pBuffer->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VertexBuffer, m_VertexBufferMemory);

	pBuffer->CopyBuffer(stagingBuffer, m_VertexBuffer, bufferSize, pDevice->GetGraphicsQueue(), pCommandManager);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Scene::CreateIndexBuffer(GG::Device* pDevice, const GG::Buffer* pBuffer, const GG::CommandManager* pCommandManager)
{
	const auto& device = pDevice->GetVulkanDevice();

	VkDeviceSize bufferSize = sizeof(m_SceneIndices[0]) * m_SceneIndices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	pBuffer->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, m_SceneIndices.data(), (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	pBuffer->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_IndexBuffer, m_IndexBufferMemory);

	pBuffer->CopyBuffer(stagingBuffer, m_IndexBuffer, bufferSize, pDevice->GetGraphicsQueue(), pCommandManager);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Scene::Destroy(const VkDevice device) const
{
	vkDestroyBuffer(device, m_IndexBuffer, nullptr);
	vkFreeMemory(device, m_IndexBufferMemory, nullptr);

	vkDestroyBuffer(device, m_VertexBuffer, nullptr);
	vkFreeMemory(device, m_VertexBufferMemory, nullptr);
}
