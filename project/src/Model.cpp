#include "Model.h"

#include "GGBuffer.h"
#include "GGVkDevice.h"

void Mesh::CreateBuffers(GG::Device* pDevice, const GG::Buffer* pBuffer, const GG::CommandManager* pCommandManager)
{
	CreateVertexBuffer(pDevice, pBuffer, pCommandManager);
	CreateIndexBuffer(pDevice, pBuffer, pCommandManager);
}

void Mesh::Destroy(const VkDevice device) const
{
	vkDestroyBuffer(device, m_IndexBuffer, nullptr);
	vkFreeMemory(device, m_IndexBufferMemory, nullptr);

	vkDestroyBuffer(device, m_VertexBuffer, nullptr);
	vkFreeMemory(device, m_VertexBufferMemory, nullptr);
}

void Mesh::SetModelMatrix(const glm::mat4& modelMatrix)
{
	m_ModelMatrix = modelMatrix;
}

void Mesh::SetModelMatrix(const aiMatrix4x4& modelMatrix)
{
	m_ModelMatrix = glm::mat4(
		modelMatrix.a1, modelMatrix.b1, modelMatrix.c1,modelMatrix.d1,
		modelMatrix.a2, modelMatrix.b2, modelMatrix.c2,modelMatrix.d2,
		modelMatrix.a3, modelMatrix.b3, modelMatrix.c3,modelMatrix.d3,
		modelMatrix.a4, modelMatrix.b4, modelMatrix.c4,modelMatrix.d4
	);;
}

void Mesh::CreateVertexBuffer(GG::Device* pDevice, const GG::Buffer* pBuffer, const GG::CommandManager* pCommandManager)
{
	const auto& device = pDevice->GetVulkanDevice();

	VkDeviceSize bufferSize = sizeof(m_Vertices[0]) * m_Vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	pBuffer->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, m_Vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	pBuffer->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VertexBuffer, m_VertexBufferMemory);

	pBuffer->CopyBuffer(stagingBuffer, m_VertexBuffer, bufferSize, pDevice->GetGraphicsQueue(), pCommandManager);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Mesh::CreateIndexBuffer(GG::Device* pDevice, const GG::Buffer* pBuffer, const GG::CommandManager* pCommandManager)
{
	const auto& device = pDevice->GetVulkanDevice();

	VkDeviceSize bufferSize = sizeof(m_Indices[0]) * m_Indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	pBuffer->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, m_Indices.data(), (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	pBuffer->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_IndexBuffer, m_IndexBufferMemory);

	pBuffer->CopyBuffer(stagingBuffer, m_IndexBuffer, bufferSize, pDevice->GetGraphicsQueue(), pCommandManager);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}
