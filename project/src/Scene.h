#pragma once
#include "Model.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"

namespace GG
{
	class CommandManager;
	class Buffer;
	class Device;
}

class Scene
{
public:
	void AddFileToScene(const std::string& filePath);
	void AddFilesToScene(const std::initializer_list<const std::string>& filePath);
	static void ProcessNode(const aiNode* node, const aiScene* scene, std::vector<Mesh>& meshes, const std::string& modelDirectory);
	static Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene, const std::string& modelDirectory);

	void CreateMeshBuffers(GG::Device* pDevice, const GG::Buffer* pBuffer, const GG::CommandManager* pCommandManager);

	std::vector<Mesh>& GetMeshes(){return m_Models;}

	void Destroy(const VkDevice& device) const;
private:
	std::vector<Mesh> m_Models;
};
