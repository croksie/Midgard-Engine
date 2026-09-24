#pragma once
#include <memory>
#include <string>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "bifrost/bifrost.h"
#include "bifrost/buffer.h"
#include "bifrost/pipeline.h"

#include "resources/mesh.h"
#include "resources/material.h"
#include "scene/model.h"
#include "utils/shader_compiler.h"
#include "utils/file_utils.h"
#include "utils/log.h"

namespace midgard::utils::importer {

inline scene::Model importModel(const std::string& pFile, bifrost::Bifrost* bifrost, bool useInstance = false) {
    // Import file
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile( pFile,
    aiProcess_CalcTangentSpace       |
    aiProcess_Triangulate            |
    aiProcess_JoinIdenticalVertices  |
    aiProcess_SortByPType);

    // If the import failed, report it
    if (nullptr == scene) {
        ENGINE_LOG_ERROR("Failed to import model: {}", importer.GetErrorString());
        return {};
    }
    auto ai_mesh = scene->mMeshes[0];

    std::vector<bifrost::Vertex> vertices;
    std::vector<uint32_t> indices;
    vertices.reserve(ai_mesh->mNumVertices);
    // Vertices
    for (unsigned int i = 0; i < ai_mesh->mNumVertices; ++i) {
        bifrost::Vertex vertex{};
        // Position
        vertex.position = math::Vec3(ai_mesh->mVertices[i].x, ai_mesh->mVertices[i].y, ai_mesh->mVertices[i].z);

        // Normals
        if (ai_mesh->HasNormals()) {
            vertex.normals = math::Vec3(ai_mesh->mNormals[i].x, ai_mesh->mNormals[i].y, ai_mesh->mNormals[i].z);
        }

        // Text Coords
        if (ai_mesh->mTextureCoords[0]) {
            vertex.texCoords = math::Vec2(ai_mesh->mTextureCoords[0][i].x, ai_mesh->mTextureCoords[0][i].y);
        } else {
            vertex.texCoords = math::Vec2(0.0f, 0.0f);
        }
        vertices.push_back(vertex);
    }
    // Indices
    for (unsigned int i = 0; i < ai_mesh->mNumFaces; ++i) {
        aiFace face = ai_mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // Mesh buffers
    bifrost::BufferDesc vertexBufferDesc{};
    vertexBufferDesc.type = bifrost::BufferType::VERTEX;
    vertexBufferDesc.size = vertices.size() * sizeof(bifrost::Vertex);
    vertexBufferDesc.initData = vertices.data();
    auto vertexBuffer = bifrost->createBuffer(vertexBufferDesc);

    bifrost::BufferDesc indexBufferDesc{};
    indexBufferDesc.type = bifrost::BufferType::INDEX;
    indexBufferDesc.size = indices.size() * sizeof(uint32_t);
    indexBufferDesc.initData = indices.data();
    auto indexBuffer = bifrost->createBuffer(indexBufferDesc);

    auto mesh = std::make_shared<resource::Mesh>(vertexBuffer, indexBuffer);

    // Material
    std::string assetFolder(ASSETS_PATH);
    bifrost::PipelineInfo pipelineInfo;

    std::string shaderPath = assetFolder + "shaders/base.hlsl";

    pipelineInfo.vertexShader = bifrost->createShader(bifrost::ShaderType::VERTEX, utils::file::readTextFile(shaderPath), useInstance);
    pipelineInfo.fragmentShader = bifrost->createShader(bifrost::ShaderType::FRAGMENT, utils::file::readTextFile(shaderPath), false);
    pipelineInfo.useInstance = useInstance;

    auto pipeline = bifrost->createPipeline(pipelineInfo);
    auto material = std::make_shared<resource::Material>(pipeline);

    return scene::Model(mesh, material);
}

}; // namespace midgard::utils::importer
