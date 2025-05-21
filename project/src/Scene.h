#pragma once
#include "GGCamera.h"
#include "Model.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"

namespace GG
{
	class CommandManager;
	class Buffer;
	class Device;
	class CommandManager;
}

class Scene
{
public:
	Scene();

	void Initialize(GLFWwindow* window);
	void AddFileToScene(const std::string& filePath);
	void AddFilesToScene(const std::initializer_list<const std::string>& filePath);
	void BindTextureToMesh(const std::string& modelFilePath, const std::string& textureFilePath);
	void ProcessNode(const aiNode* node, const aiScene* scene, const std::string& modelDirectory);
	Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene, const std::string& modelDirectory);

	void Update();

	void CreateMeshBuffers(GG::Device* pDevice, const GG::Buffer* pBuffer, const GG::CommandManager* pCommandManager);
	void CreateImages(GG::Buffer* buffer, const GG::CommandManager* commandManager, VkQueue graphicsQueue, VkDevice device, VkPhysicalDevice physicalDevice) const;

	std::vector<Mesh>& GetMeshes(){return m_Models;}
	std::vector<GG::Texture*> GetTextures() { return m_Textures; }
	std::vector<VkImageView> GetImageViews() const;
	int GetTextureCount() const { return m_Textures.size(); }
	GG::Camera& GetCamera() { return m_Camera; }

	void Destroy(const VkDevice& device) const;
	glm::mat4 GetSceneMatrix() const { return m_SceneMatrix; }

private:
	std::unordered_map<std::string, int> m_ModelPaths;
	GG::Camera m_Camera;
	std::vector<Mesh> m_Models;
	std::vector<GG::Texture*> m_Textures;
	glm::mat4 m_SceneMatrix { glm::rotate(glm::mat4(1.0f), glm::radians(0.f), glm::vec3(0.0f, 0.0f, 1.0f)) };
	std::unordered_map<std::string, int> m_TexturePaths;
};
