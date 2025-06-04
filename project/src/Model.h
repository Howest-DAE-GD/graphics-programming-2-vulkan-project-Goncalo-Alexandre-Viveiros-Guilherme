#pragma once
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <array>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include "assimp/matrix4x4.h"


class Scene;

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec3 bitangent;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 6> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 6> attributeDescriptions{};

		// Position
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		// Color
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		// Texture coordinates 
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		// Normal
		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(Vertex, normal);

		// Tangent
		attributeDescriptions[4].binding = 0;
		attributeDescriptions[4].location = 4;
		attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[4].offset = offsetof(Vertex, tangent);

		// BiTangent
		attributeDescriptions[5].binding = 0;
		attributeDescriptions[5].location = 5;
		attributeDescriptions[5].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[5].offset = offsetof(Vertex, bitangent);

		return attributeDescriptions;
	}

	bool operator==(const Vertex& other) const
	{
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}
};

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}

namespace GG
{
	class Texture;
	class CommandManager;
	class Buffer;
	class Device;
}

class Mesh
{
public:
	struct PBRMaterialIndices {
		uint32_t albedoTexIdx = 0;           // Index into m_Textures for Base Color/Albedo
		uint32_t normalTexIdx = 0;           // Index into m_Textures for Normal Map
		uint32_t metallicRoughnessTexIdx = 0; // Index into m_Textures for Metallic/Roughness Map
		uint32_t aoTexIdx = 0;               // Index into m_Textures for Ambient Occlusion Map
		// Add more if you have specific emission, height, etc.
	};

	const PBRMaterialIndices& GetMaterialIndices() const { return m_MaterialIndices; }
	void SetMaterialIndices(const PBRMaterialIndices& indices) { m_MaterialIndices = indices; }

	void CreateVertexBuffer(GG::Device* pDevice, const GG::Buffer* pBuffer, const GG::CommandManager* pCommandManager);
	void CreateIndexBuffer(GG::Device* pDevice, const GG::Buffer* pBuffer, const GG::CommandManager* pCommandManager);

	void CreateBuffers(GG::Device* pDevice, const GG::Buffer* pBuffer, const GG::CommandManager* pCommandManager);
	void Destroy(VkDevice device) const;

	std::vector<Vertex>& GetVertices() { return m_Vertices; }
	std::vector<uint32_t>& GetIndices() { return m_Indices; }

	VkBuffer GetVertexBuffer() const {return m_VertexBuffer;}
	VkBuffer GetIndexBuffer() const { return m_IndexBuffer; }

	void SetParentScene(Scene* scene) { m_pParentScene = scene; }

	void SetTextureIdx(int idx) { m_TextureIndex = idx; }
	int GetTextureIdx() const { return m_MaterialIndices.albedoTexIdx; }

	void SetModelMatrix(const glm::mat4& modelMatrix);
	void SetModelMatrix(const aiMatrix4x4& modelMatrix);
	glm::mat4 GetModelMatrix() const { return m_ModelMatrix; }

private:
	PBRMaterialIndices m_MaterialIndices;
	std::vector<Vertex> m_Vertices;
	std::vector<uint32_t> m_Indices;

	const std::string m_ModelPath;
//	std::string m_ModelTexture;
//
//	GG::Texture* m_Texture;

	Scene* m_pParentScene;
	int m_TextureIndex;
	glm::mat4 m_ModelMatrix;

	VkBuffer m_VertexBuffer				= VK_NULL_HANDLE;
	VkDeviceMemory m_VertexBufferMemory = VK_NULL_HANDLE;

	VkBuffer m_IndexBuffer				= VK_NULL_HANDLE;
	VkDeviceMemory m_IndexBufferMemory	= VK_NULL_HANDLE;

};
