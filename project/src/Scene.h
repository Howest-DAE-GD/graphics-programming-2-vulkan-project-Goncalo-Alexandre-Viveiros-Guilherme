#pragma once
#include "Model.h"

class Scene
{
public:
	std::vector<Vertex> GetSceneVertices() { return m_SceneVertices; }
	std::vector<uint32_t> GetSceneIndices() { return m_SceneIndices; }
	void AddModel(const Model& modelToAdd);
	void AddModel(const std::initializer_list<Model>& modelsToAdd);

	VkBuffer& GetVertexBuffer() { return m_VertexBuffer; }
	VkBuffer& GetIndexBuffer() { return m_IndexBuffer; }
	VkDeviceMemory& GetVertexBufferMemory() { return m_VertexBufferMemory; }
	VkDeviceMemory& GetIndexBufferMemory() { return m_IndexBufferMemory; }

	void Destroy(VkDevice device) const;
private:
	std::vector<Model> m_Models;
	std::vector<Vertex> m_SceneVertices;
	std::vector<uint32_t> m_SceneIndices;

	VkBuffer m_VertexBuffer;
	VkDeviceMemory m_VertexBufferMemory;
	VkBuffer m_IndexBuffer;
	VkDeviceMemory m_IndexBufferMemory;
};