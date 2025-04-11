#pragma once
#include "Model.h"

class Scene
{
public:
	std::vector<Vertex> GetSceneVertices() { return m_SceneVertices; }
	std::vector<uint32_t> GetSceneIndices() { return m_SceneIndices; }
	void AddModel(const Model& modelToAdd);
	void AddModel(const std::initializer_list<Model>& modelsToAdd);


private:
	std::vector<Model> m_Models;
	std::vector<Vertex> m_SceneVertices;
	std::vector<uint32_t> m_SceneIndices;
};