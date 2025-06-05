#pragma once
#include <memory>

#include "GGCamera.h"
#include "GGTexture.h"
#include "Model.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"

namespace GG
{
	class CommandManager;
	class Buffer;
	class Device;
}

struct alignas(16) DirectionalLight {
	glm::vec3 Direction;  
	float Intensity;       
	glm::vec3 Color;      
};

struct alignas(16) PointLight
{
	glm::vec3 Position;
	float Radius;
	glm::vec3 Color;
};

class Scene
{
public:
	Scene();

	void Initialize(GLFWwindow* window);
	void AddFileToScene(const std::string& filePath);
	void AddFilesToScene(const std::initializer_list<const std::string>& filePath);
	void AddLight(PointLight lightToAdd);
	void AddLight(DirectionalLight lightToAdd);
	void BindTextureToMesh(const std::string& modelFilePath, const std::string& textureFilePath, VkFormat imgFormat);
	void ProcessNode(const aiNode* node, const aiScene* scene, const std::string& modelDirectory);
	Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene, const std::string& modelDirectory);

	void Update();

	void CreateMeshBuffers(GG::Device* pDevice, const GG::Buffer* pBuffer, const GG::CommandManager* pCommandManager);
	void CreateImages(GG::Buffer* buffer, const GG::CommandManager* commandManager, VkQueue graphicsQueue, VkDevice device, VkPhysicalDevice physicalDevice) const;

	std::vector<Mesh>& GetMeshes(){return m_Models;}
	std::vector<PointLight>& GetPointLights() { return m_PointLights; }
	std::vector<DirectionalLight>& GetDirectionalLights() { return m_DirectionalLights; }

	const std::vector<std::unique_ptr<GG::Texture>>& GetTextures() const { return m_Textures; }

	uint32_t GetOrLoadTexture(const std::string& texturePath,const aiScene* scene,const std::string& modelDirectory,
		std::vector<std::unique_ptr<GG::Texture>>& textures,std::unordered_map<std::string, uint32_t>& texturePaths, VkFormat imgFormat);

	uint32_t GetOrLoadTextureFromMemory(const aiTexture* aiTex,std::vector<std::unique_ptr<GG::Texture>>& textures,
		std::unordered_map<std::string, uint32_t>& texturePaths,const std::string& fallbackKey, VkFormat imgFormat);

	std::vector<VkImageView> GetImageViews() const;

	uint32_t GetTextureCount() const { return static_cast<uint32_t>(m_Textures.size()); }

	GG::Camera& GetCamera() { return m_Camera; }

	void Destroy(const VkDevice& device) const;
	glm::mat4 GetSceneMatrix() const { return m_SceneMatrix; }

private:
	std::unordered_map<std::string, int> m_ModelPaths;
	GG::Camera m_Camera;
	std::vector<Mesh> m_Models;
	std::vector<PointLight> m_PointLights;
	std::vector<DirectionalLight> m_DirectionalLights;
	std::vector<std::unique_ptr<GG::Texture>> m_Textures;
	glm::mat4 m_SceneMatrix { glm::rotate(glm::mat4(1.0f), glm::radians(0.f), glm::vec3(0.0f, 0.0f, 1.0f)) };
	std::unordered_map<std::string, uint32_t> m_TexturePaths;
};
