#include "Scene.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>

#include "GGBuffer.h"
#include "tiny_obj_loader.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"


Scene::Scene()
{
	// Index 0: Missing/Default Albedo
	m_Textures.emplace_back(std::make_unique<GG::Texture>("resources/textures/missingTexture.png", VK_FORMAT_R8G8B8A8_SRGB));
	m_TexturePaths.emplace("resources/textures/missingTexture.png", 0);

    // Default Normal Map
    auto normalPixels = std::make_unique<stbi_uc[]>(4);
    normalPixels[0] = 127;
    normalPixels[1] = 127;
    normalPixels[2] = 255;
    normalPixels[3] = 255;
    m_Textures.emplace_back(std::make_unique<GG::Texture>(
        std::move(normalPixels), 1, 1, VK_FORMAT_R8G8B8A8_UNORM));
    m_TexturePaths.emplace("default_normal_map", 1);

    // Default MetallicRoughness Map
    auto mrPixels = std::make_unique<stbi_uc[]>(4);
    mrPixels[0] = 0;
    mrPixels[1] = 255;
    mrPixels[2] = 0;
    mrPixels[3] = 0;
    m_Textures.emplace_back(std::make_unique<GG::Texture>(
        std::move(mrPixels), 1, 1, VK_FORMAT_R8G8B8A8_UNORM));
    m_TexturePaths.emplace("default_metallic_roughness_map", 2);

    // Default AO Map
    auto aoPixels = std::make_unique<stbi_uc[]>(4);
    aoPixels[0] = 255;
    aoPixels[1] = 255;
    aoPixels[2] = 255;
    aoPixels[3] = 255;
    m_Textures.emplace_back(std::make_unique<GG::Texture>(
        std::move(aoPixels), 1, 1, VK_FORMAT_R8G8B8A8_SRGB));
    m_TexturePaths.emplace("default_ao_map", 3);

    AddLight(PointLight{ {0,0,0},0,{0,0,0} }); //Default 0 Lights to avoid buffers throwing errors
    AddLight(DirectionalLight{ {0,0,0},0,{0,0,0} });
}

void Scene::Initialize(GLFWwindow* window)
{
    m_Camera.Initialize(45, glm::vec3(0.f, 0.f, 0.f), 13.f / 9.f, window);
}

void Scene::AddFileToScene(const std::string& filePath)
{
	Assimp::Importer importer;


    const aiScene* scene =
        importer.ReadFile(filePath,
            aiProcess_Triangulate |
            aiProcess_OptimizeMeshes |
            aiProcess_FlipUVs |
            aiProcess_JoinIdenticalVertices |
            aiProcess_SortByPType |
            aiProcess_CalcTangentSpace
        );


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

void Scene::AddLight(PointLight lightToAdd)
{
    m_PointLights.emplace_back(lightToAdd);
}

void Scene::AddLight(DirectionalLight lightToAdd)
{
    m_DirectionalLights.emplace_back(lightToAdd);
}

void Scene::BindTextureToMesh(const std::string& modelFilePath, const std::string& textureFilePath, VkFormat imgFormat)
{
	if (m_ModelPaths.contains(modelFilePath))
	{
        m_Textures.emplace_back(new GG::Texture(textureFilePath, imgFormat));
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
	for (uint32_t i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		m_Models.push_back(ProcessMesh(mesh, scene, modelDirectory));
        m_ModelPaths.emplace(modelDirectory, m_Models.size());
	}

	for (uint32_t i = 0; i < node->mNumChildren; i++)
	{
		ProcessNode(node->mChildren[i], scene, modelDirectory);
	}
}

Mesh Scene::ProcessMesh(aiMesh* mesh, const aiScene* scene, const std::string& modelDirectory)
{
    Mesh newMesh{};
    Mesh::PBRMaterialIndices materialIndices;

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

        // Normals
        if (mesh->HasNormals())
        {
            vertex.normal = {
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            };
        }
        else {
            vertex.normal = { 0.0f, 0.0f, 0.0f };
        }

        // Tangents
        if (mesh->HasTangentsAndBitangents()) {
            vertex.tangent = {
                mesh->mTangents[i].x,
                mesh->mTangents[i].y,
                mesh->mTangents[i].z
            };

            vertex.bitangent = {
                 mesh->mBitangents[i].x,
                 mesh->mBitangents[i].y,
                 mesh->mBitangents[i].z
            };
        }
        else {
            vertex.tangent = { 0.0f, 1.0f, 0.0f };
            vertex.bitangent = { 0.0f, 0.0f, 0.0f };
        }

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

    // Materials (PBR Textures)
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    std::string modelBaseDir = std::filesystem::path(modelDirectory).parent_path().string();


    // Albedo (Base Color) Map
    aiString path;
    if (material->GetTexture(aiTextureType_BASE_COLOR, 0, &path) == AI_SUCCESS)
    {
        materialIndices.albedoTexIdx = GetOrLoadTexture(path.C_Str(), scene, modelBaseDir, m_Textures, m_TexturePaths, VK_FORMAT_R8G8B8A8_SRGB);
    }
    else if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS)
    {
        materialIndices.albedoTexIdx = GetOrLoadTexture(path.C_Str(), scene, modelBaseDir, m_Textures, m_TexturePaths, VK_FORMAT_R8G8B8A8_SRGB);
    }
    else {
        materialIndices.albedoTexIdx = 0; 
    }

   // Normal Map
   if (material->GetTexture(aiTextureType_NORMAL_CAMERA, 0, &path) == AI_SUCCESS)
   {
       materialIndices.normalTexIdx = GetOrLoadTexture(path.C_Str(), scene, modelBaseDir, m_Textures, m_TexturePaths, VK_FORMAT_R8G8B8A8_UNORM);
   }
   else if (material->GetTexture(aiTextureType_NORMALS, 0, &path) == AI_SUCCESS) 
   {
       materialIndices.normalTexIdx = GetOrLoadTexture(path.C_Str(), scene, modelBaseDir, m_Textures, m_TexturePaths, VK_FORMAT_R8G8B8A8_UNORM);
   }
   else 
   {
       materialIndices.normalTexIdx = 1;
   }

   // Metalness Roughness Map
   if (material->GetTexture(aiTextureType_METALNESS, 0, &path) == AI_SUCCESS)
   {
       materialIndices.metallicRoughnessTexIdx = GetOrLoadTexture(path.C_Str(), scene, modelBaseDir, m_Textures, m_TexturePaths, VK_FORMAT_R8G8B8A8_UNORM);
   }
   else if (material->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &path) == AI_SUCCESS) 
   {
       materialIndices.metallicRoughnessTexIdx = GetOrLoadTexture(path.C_Str(), scene, modelBaseDir, m_Textures, m_TexturePaths, VK_FORMAT_R8G8B8A8_UNORM);
   }
   else 
   {
       materialIndices.metallicRoughnessTexIdx = 2;
   }

   // Ambient Occlusion Map
   //if (material->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0, &path) == AI_SUCCESS)
   //{
   //    materialIndices.aoTexIdx = GetOrLoadTexture(path.C_Str(), scene, modelBaseDir, m_Textures, m_TexturePaths, VK_FORMAT_R8G8B8A8_SRGB);
   //}
   //else if (material->GetTexture(aiTextureType_LIGHTMAP, 0, &path) == AI_SUCCESS) 
   //{
   //    materialIndices.aoTexIdx = GetOrLoadTexture(path.C_Str(), scene, modelBaseDir, m_Textures, m_TexturePaths, VK_FORMAT_R8G8B8A8_SRGB);
   //}
   //else 
   //{
   //    materialIndices.aoTexIdx = 3; 
   //}

    newMesh.SetMaterialIndices(materialIndices);
    newMesh.SetModelMatrix(scene->mRootNode->mTransformation);

    newMesh.SetParentScene(this);

    return newMesh;
}

uint32_t Scene::GetOrLoadTexture(const std::string& textureFileName, const aiScene* scene,const std::string& modelDirectory, 
    std::vector<std::unique_ptr<GG::Texture>>& textures,std::unordered_map<std::string, uint32_t>& texturePaths, VkFormat imgFormat)
{
    std::string texId = textureFileName;

    if (!texId.empty() && texId[0] == '*') 
    {
        unsigned int embeddedIndex = std::stoi(texId.substr(1));
        aiTexture* aiTex = scene->mTextures[embeddedIndex];
        return GetOrLoadTextureFromMemory(aiTex, textures, texturePaths, textureFileName,imgFormat);
    }
    else
    {
        std::filesystem::path modelPath = modelDirectory;
        std::filesystem::path resolvePath = modelPath / textureFileName;
        std::string fullTexturePath = resolvePath.string();

        if (texturePaths.count(fullTexturePath)) 
        {
            return texturePaths[fullTexturePath];
        }
        else 
        {
            if (std::filesystem::exists(fullTexturePath)) 
            {
                textures.emplace_back(std::make_unique<GG::Texture>(fullTexturePath,imgFormat));
                uint32_t newIndex = static_cast<uint32_t>(textures.size() - 1);
                texturePaths.emplace(fullTexturePath, newIndex);
                return newIndex;
            }
            else 
            {
                std::cerr << "WARNING: Texture file not found: " << fullTexturePath << "\n";
                return 0; 
            }
        }
    }
}

// In Scene.cpp
uint32_t Scene::GetOrLoadTextureFromMemory(const aiTexture* aiTex,
    std::vector<std::unique_ptr<GG::Texture>>& textures,
    std::unordered_map<std::string, uint32_t>& texturePaths,
    const std::string& fallbackKey, VkFormat imgFormat)
{
    if (texturePaths.count(fallbackKey)) {
        return texturePaths[fallbackKey];
    }

    int32_t texW, texH, texChannels;
    std::unique_ptr<stbi_uc[]> pixels;

    if (aiTex->mHeight == 0) {
        // Compressed texture data
        stbi_uc* loadedPixels = stbi_load_from_memory(
            reinterpret_cast<stbi_uc*>(aiTex->pcData),
            aiTex->mWidth,
            &texW, &texH, &texChannels, STBI_rgb_alpha);

        if (loadedPixels) {
            pixels.reset(loadedPixels);
        }
    }
    else {
        // Uncompressed texture data
        size_t bufferSize = aiTex->mWidth * aiTex->mHeight * 4;
        pixels = std::make_unique<stbi_uc[]>(bufferSize);

        if (pixels) {
            for (uint32_t y = 0; y < aiTex->mHeight; y++) {
                for (uint32_t x = 0; x < aiTex->mWidth; x++) {
                    const aiTexel* texel = &aiTex->pcData[y * aiTex->mWidth + x];
                    uint32_t idx = (y * aiTex->mWidth + x) * 4;
                    pixels[idx] = texel->r;
                    pixels[idx + 1] = texel->g;
                    pixels[idx + 2] = texel->b;
                    pixels[idx + 3] = texel->a;
                }
            }
        }
        texW = aiTex->mWidth;
        texH = aiTex->mHeight;
    }

    if (pixels) {
        try {
            auto embeddedTex = std::make_unique<GG::Texture>(std::move(pixels), texW, texH, imgFormat);
            textures.emplace_back(std::move(embeddedTex));
            uint32_t newIndex = static_cast<uint32_t>(textures.size() - 1);
            texturePaths.emplace(fallbackKey, newIndex);
            return newIndex;
        }
        catch (...) {
            std::cerr << "ERROR: Failed to create embedded texture: " << fallbackKey << "\n";
            return 0;
        }
    }
    else {
        std::cerr << "WARNING: Failed to load embedded texture: " << fallbackKey << "\n";
        return 0;
    }
}
void Scene::Update()
{
    m_Camera.Update();
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
    for (auto& texture : m_Textures)
	{
        texture->CreateImage(buffer, commandManager, graphicsQueue, device, physicalDevice);
	}
}

std::vector<VkImageView> Scene::GetImageViews() const
{
    std::vector<VkImageView> imageViews;
	imageViews.reserve(m_Textures.size());

    for (const auto& texture : m_Textures)
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
	for (const auto& texture : m_Textures)
	{
        texture->DestroyTexture(device);
	}
}
