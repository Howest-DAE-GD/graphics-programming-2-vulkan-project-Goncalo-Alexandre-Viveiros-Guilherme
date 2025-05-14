#include "Scene.h"

#include <iostream>
#include <stdexcept>

#include "GGBuffer.h"
#include "GGTexture.h"
#include "GGVkDevice.h"
#include "tiny_obj_loader.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

void Scene::AddFileToScene(const std::string& filePath)
{
	Assimp::Importer importer;

	const aiScene* scene =
		importer.ReadFile(filePath,
			aiProcess_Triangulate |
			aiProcess_OptimizeMeshes |
			aiProcess_FlipUVs |
			aiProcess_JoinIdenticalVertices |
			aiProcess_SortByPType);


	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cerr << "ASSIMP ERROR: " << importer.GetErrorString() << "\n";
		return;
	}
	ProcessNode(scene->mRootNode,scene, filePath);
}

void Scene::AddFilesToScene(const std::initializer_list<const std::string>& filePath)
{
	for (auto& pFile : filePath)
	{
		AddFileToScene(pFile);
	}
}

void Scene::BindTextureToMesh(const std::string& modelFilePath, const std::string& textureFilePath)
{
	if (m_ModelPaths.contains(modelFilePath))
	{
        m_Textures.emplace_back(new GG::Texture(textureFilePath));
        m_TexturePaths.emplace(textureFilePath, static_cast<int>(m_Textures.size() - 1));

        m_Models[m_ModelPaths.find(modelFilePath)->second - 1].SetTextureIdx(static_cast<int>( m_Textures.size() - 1 ));
	}
	else
	{
        throw std::runtime_error("Cannot bind texture to mesh!No mesh found with that file path.");
	}
}

void Scene::ProcessNode(const aiNode* node, const aiScene* scene, const std::string& modelDirectory)
{
	// Process all the meshes in this node
	for (uint32_t i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		m_Models.push_back(ProcessMesh(mesh, scene, modelDirectory));
        m_ModelPaths.emplace(modelDirectory, m_Models.size());
	}

	// Recursively process children
	for (uint32_t i = 0; i < node->mNumChildren; i++)
	{
		ProcessNode(node->mChildren[i], scene, modelDirectory);
	}
}

Mesh Scene::ProcessMesh(aiMesh* mesh, const aiScene* scene, const std::string& modelDirectory)
{
    Mesh newMesh{};

    // Vertices
    for (uint32_t i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex{};

        // Positions
        vertex.pos = {
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        };

        //if (mesh->HasNormals())
        //{
        //    vertex.normal = {
        //        mesh->mNormals[i].x,
        //        mesh->mNormals[i].y,
        //        mesh->mNormals[i].z
        //    };
        //}

        // Texture coordinates
        if (mesh->mTextureCoords[0]) 
        {
            vertex.texCoord = {
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y
            };
        }
        else
        {
            vertex.texCoord = { 0.0f, 0.0f };
        }

        vertex.color = { 1.0f, 1.0f, 1.0f };

        newMesh.GetVertices().push_back(vertex);
    }

    // Indices
    for (uint32_t i = 0; i < mesh->mNumFaces; i++)
    {
        const aiFace& face = mesh->mFaces[i];
        for (uint32_t j = 0; j < face.mNumIndices; j++)
        {
            newMesh.GetIndices().push_back(face.mIndices[j]);
        }
    }

    // Materials (textures)
//    if (mesh->mMaterialIndex >= 0)
//    {
//        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
//
//        aiString path;
//        if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS)
//        {
//            std::string textureFile = path.C_Str();
//            std::string texturePath = modelDirectory + "/" + textureFile;
//            if (!m_TexturePaths.contains(texturePath))
//            {
//                //if it doesnt contain that texture already
//                m_Textures.emplace_back(new GG::Texture(texturePath));
//                m_TexturePaths.emplace(texturePath, static_cast<int>(m_Textures.size() - 1));
//                newMesh.SetTextureIdx(static_cast<int>(m_Textures.size() - 1));
//            }
//            else
//            {
//	            //if it does
//                newMesh.SetTextureIdx(m_TexturePaths.find(texturePath)->second);
//            }
//           
//        }
//    }

    newMesh.SetParentScene(this);

    return newMesh;
}

void Scene::CreateMeshBuffers(GG::Device* pDevice, const GG::Buffer* pBuffer, const GG::CommandManager* pCommandManager)
{
    for (auto& model : m_Models)
    {
        model.CreateBuffers(pDevice, pBuffer, pCommandManager);
    }
}

void Scene::CreateImages(GG::Buffer* buffer, const GG::CommandManager* commandManager, VkQueue graphicsQueue,
	VkDevice device, VkPhysicalDevice physicalDevice) const
{
	for (auto texture : m_Textures)
	{
        texture->CreateImage(buffer, commandManager, graphicsQueue, device, physicalDevice);
	}
}

std::vector<VkImageView> Scene::GetImageViews() const
{
    std::vector<VkImageView> imageViews;
	imageViews.reserve(m_Textures.size());

    for (const auto texture : m_Textures)
    {
	    imageViews.emplace_back(texture->GetImageView());
	}

    return imageViews;
}

void Scene::Destroy(const VkDevice& device) const
{
	for (auto& model : m_Models)
	{
		model.Destroy(device);
	}
	for (const auto texture : m_Textures)
	{
        texture->DestroyTexture(device);
	}
}
