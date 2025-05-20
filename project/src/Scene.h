#pragma once
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

	void AddFileToScene(const std::string& filePath);
	void AddFilesToScene(const std::initializer_list<const std::string>& filePath);
	void BindTextureToMesh(const std::string& modelFilePath, const std::string& textureFilePath);
	void ProcessNode(const aiNode* node, const aiScene* scene, const std::string& modelDirectory);
	Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene, const std::string& modelDirectory);

	void CreateMeshBuffers(GG::Device* pDevice, const GG::Buffer* pBuffer, const GG::CommandManager* pCommandManager);
	void CreateImages(GG::Buffer* buffer, const GG::CommandManager* commandManager, VkQueue graphicsQueue, VkDevice device, VkPhysicalDevice physicalDevice) const;

	std::vector<Mesh>& GetMeshes(){return m_Models;}
	std::vector<GG::Texture*> GetTextures() { return m_Textures; }
	std::vector<VkImageView> GetImageViews() const;
	int GetTextureCount() const { return m_Textures.size(); }

	void Destroy(const VkDevice& device) const;
private:
	std::unordered_map<std::string, int> m_ModelPaths;
	std::vector<Mesh> m_Models;
	std::vector<GG::Texture*> m_Textures;
	std::unordered_map<std::string, int> m_TexturePaths;
};
