#include "resources/model_importer.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <numbers>
#include <numeric>
#include <string>
#include <unordered_map>
#include <vector>

#include <assimp/GltfMaterial.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "math/math.h"
#include "utils/file_utils.h"
#include "utils/log.h"

#include "bifrost/bifrost.h"
#include "bifrost/bind_group.h"
#include "bifrost/buffer.h"
#include "bifrost/pipeline.h"
#include "bifrost/pipeline_manager.h"
#include "bifrost/shader.h"
#include "bifrost/texture.h"

#include "resources/default.h"
#include "resources/material.h"
#include "resources/mesh.h"
#include "scene/model.h"
#include "scene/scene_node.h"

namespace midgard::resource {

namespace {

math::Transform convertTransform(const aiMatrix4x4& matrix) {
    aiVector3D scale;
    aiVector3D position;
    aiQuaternion rotation;
    matrix.Decompose(scale, rotation, position);

    const aiMatrix3x3 ROTATION_MATRIX = rotation.GetMatrix();

    const float SIN_Y = std::clamp(ROTATION_MATRIX.a3, -1.0F, 1.0F);
    const float ANGLE_Y = std::asin(SIN_Y);
    float angleX = 0.0F;
    float angleZ = 0.0F;

    if (std::abs(SIN_Y) < 0.99999F) {
        angleX = std::atan2(-ROTATION_MATRIX.b3, ROTATION_MATRIX.c3);
        angleZ = std::atan2(-ROTATION_MATRIX.a2, ROTATION_MATRIX.a1);
    } else {
        angleX = std::atan2(SIN_Y * ROTATION_MATRIX.b1, ROTATION_MATRIX.b2);
    }

    constexpr float RAD_TO_DEG = 180.0F / std::numbers::pi_v<float>;

    math::Transform transform;
    transform.position = math::Vec3(position.x, position.y, position.z);
    transform.rotation = math::Vec3(angleX * RAD_TO_DEG, ANGLE_Y * RAD_TO_DEG, angleZ * RAD_TO_DEG);
    transform.scale = math::Vec3(scale.x, scale.y, scale.z);
    return transform;
}

std::shared_ptr<Mesh> processMesh(const aiMesh* aiMesh, bifrost::Bifrost* bifrost) {
    std::vector<bifrost::Vertex> vertices;
    std::vector<uint32_t> indices;

    vertices.reserve(aiMesh->mNumVertices);

    // Extract vertices
    for (unsigned int vertexIndex = 0; vertexIndex < aiMesh->mNumVertices; ++vertexIndex) {
        bifrost::Vertex vertex{};

        const aiVector3D& position = aiMesh->mVertices[vertexIndex];
        vertex.position = math::Vec3(position.x, position.y, position.z);

        if (aiMesh->HasNormals()) {
            const aiVector3D& normal = aiMesh->mNormals[vertexIndex];
            vertex.normals = math::Vec3(normal.x, normal.y, normal.z);
        } else {
            vertex.normals = math::Vec3(0.0F, 1.0F, 0.0F);
        }

        if (aiMesh->mTextureCoords[0] != nullptr) {
            const aiVector3D& texCoords = aiMesh->mTextureCoords[0][vertexIndex];
            vertex.texCoords = math::Vec2(texCoords.x, texCoords.y);
        } else {
            vertex.texCoords = math::Vec2(0.0F, 0.0F);
        }

        vertices.push_back(vertex);
    }

    // Extract indices
    for (unsigned int faceIndex = 0; faceIndex < aiMesh->mNumFaces; ++faceIndex) {
        const aiFace& face = aiMesh->mFaces[faceIndex];
        for (unsigned int index = 0; index < face.mNumIndices; ++index) {
            indices.push_back(face.mIndices[index]);
        }
    }

    // Fallback if mesh has no indices
    if (indices.empty()) {
        indices.resize(vertices.size());
        std::iota(indices.begin(), indices.end(), 0U);
    }

    // Create mesh buffers
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

    return std::make_shared<Mesh>(vertexBuffer, indexBuffer);
}

std::shared_ptr<bifrost::Texture> loadTextureFromAssimp(const aiMaterial* aiMat, const std::vector<aiTextureType>& typesToTry,
                                                        const aiScene* scene, const std::string& directory, bifrost::Bifrost* bifrost,
                                                        bifrost::TextureFormat format,
                                                        std::unordered_map<std::string, std::shared_ptr<bifrost::Texture>>& textureCache) {

    aiString texPath;
    bool foundTexture = false;

    for (aiTextureType type : typesToTry) {
        if (aiMat->GetTexture(type, 0, &texPath) == AI_SUCCESS) {
            foundTexture = true;
            break;
        }
    }

    if (!foundTexture) {
        return nullptr;
    }

    std::string textureKey = texPath.C_Str();
    auto cached = textureCache.find(textureKey);
    if (cached != textureCache.end()) {
        return cached->second;
    }

    std::shared_ptr<bifrost::Texture> texture = nullptr;

    if (const aiTexture* embeddedTex = scene->GetEmbeddedTexture(texPath.C_Str())) {
        int width = 0;
        int height = 0;
        int channels = 0;

        if (embeddedTex->mHeight == 0) {
            stbi_uc* pixels = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(embeddedTex->pcData),
                                                    static_cast<int>(embeddedTex->mWidth),
                                                    &width,
                                                    &height,
                                                    &channels,
                                                    STBI_rgb_alpha);

            if (pixels != nullptr) {
                bifrost::TextureDesc texDesc{};
                texDesc.width = static_cast<uint32_t>(width);
                texDesc.height = static_cast<uint32_t>(height);
                texDesc.format = format;
                texDesc.initialData = pixels;
                texDesc.size = static_cast<uint32_t>(width * height * 4);

                texture = bifrost->createTexture(texDesc);
                stbi_image_free(pixels);
            } else {
                ENGINE_LOG_WARN("Failed to decode embedded texture '{}': {}", textureKey, stbi_failure_reason());
            }
        } else {
            width = static_cast<int>(embeddedTex->mWidth);
            height = static_cast<int>(embeddedTex->mHeight);

            std::vector<uint8_t> rgbaBuffer(static_cast<size_t>(width) * height * 4);
            for (size_t i = 0; i < static_cast<size_t>(width) * height; ++i) {
                rgbaBuffer[i * 4 + 0] = embeddedTex->pcData[i].r;
                rgbaBuffer[i * 4 + 1] = embeddedTex->pcData[i].g;
                rgbaBuffer[i * 4 + 2] = embeddedTex->pcData[i].b;
                rgbaBuffer[i * 4 + 3] = embeddedTex->pcData[i].a;
            }

            bifrost::TextureDesc texDesc{};
            texDesc.width = static_cast<uint32_t>(width);
            texDesc.height = static_cast<uint32_t>(height);
            texDesc.format = format;
            texDesc.initialData = rgbaBuffer.data();
            texDesc.size = static_cast<uint32_t>(rgbaBuffer.size());

            texture = bifrost->createTexture(texDesc);
        }
    } else {
        std::string relPath = texPath.C_Str();
        std::replace(relPath.begin(), relPath.end(), '\\', '/');
        std::filesystem::path fullPath = std::filesystem::path(directory) / relPath;
        if (std::filesystem::exists(fullPath)) {
            int width = 0;
            int height = 0;
            int channels = 0;
            stbi_uc* pixels = stbi_load(fullPath.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);

            if (pixels != nullptr) {
                bifrost::TextureDesc texDesc{};
                texDesc.width = static_cast<uint32_t>(width);
                texDesc.height = static_cast<uint32_t>(height);
                texDesc.format = format;
                texDesc.initialData = pixels;
                texDesc.size = static_cast<uint32_t>(width * height * 4);

                texture = bifrost->createTexture(texDesc);
                stbi_image_free(pixels);
            } else {
                ENGINE_LOG_WARN("Failed to load external texture '{}': {}", fullPath.string(), stbi_failure_reason());
            }
        } else {
            ENGINE_LOG_WARN("External texture not found: {}", fullPath.string());
        }
    }

    if (texture) {
        textureCache.emplace(textureKey, texture);
    }

    return texture;
}

std::shared_ptr<Material> processMaterial(const aiMaterial* aiMat, const aiScene* scene, const std::string& directory,
                                          bifrost::Bifrost* bifrost, std::shared_ptr<bifrost::Pipeline> singlePipe,
                                          std::shared_ptr<bifrost::Pipeline> instancedPipe,
                                          std::unordered_map<std::string, std::shared_ptr<bifrost::Texture>>& textureCache) {

    // 1. Textures
    auto baseColorTex = loadTextureFromAssimp(aiMat,
                                              {aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE},
                                              scene,
                                              directory,
                                              bifrost,
                                              bifrost::TextureFormat::RGBA8_SRGB,
                                              textureCache);

    auto normalTex = loadTextureFromAssimp(aiMat,
                                           {aiTextureType_NORMALS, aiTextureType_NORMAL_CAMERA, aiTextureType_HEIGHT},
                                           scene,
                                           directory,
                                           bifrost,
                                           bifrost::TextureFormat::RGBA8_UNORM,
                                           textureCache);

    auto mrTex = loadTextureFromAssimp(
        aiMat,
        {aiTextureType_GLTF_METALLIC_ROUGHNESS, aiTextureType_DIFFUSE_ROUGHNESS, aiTextureType_METALNESS, aiTextureType_UNKNOWN},
        scene,
        directory,
        bifrost,
        bifrost::TextureFormat::RGBA8_UNORM,
        textureCache);

    auto aoTex = loadTextureFromAssimp(aiMat,
                                       {aiTextureType_AMBIENT_OCCLUSION, aiTextureType_LIGHTMAP},
                                       scene,
                                       directory,
                                       bifrost,
                                       bifrost::TextureFormat::RGBA8_UNORM,
                                       textureCache);

    auto emissiveTex = loadTextureFromAssimp(aiMat,
                                             {aiTextureType_EMISSIVE, aiTextureType_EMISSION_COLOR},
                                             scene,
                                             directory,
                                             bifrost,
                                             bifrost::TextureFormat::RGBA8_SRGB,
                                             textureCache);

    // 2. Factors
    MaterialData matData{};

    aiColor4D baseColor(1.0f, 1.0f, 1.0f, 1.0f);
    if (aiMat->Get(AI_MATKEY_BASE_COLOR, baseColor) == AI_SUCCESS) {
        matData.baseColorFactor = math::Vec4(baseColor.r, baseColor.g, baseColor.b, baseColor.a);
    } else if (aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, baseColor) == AI_SUCCESS) {
        matData.baseColorFactor = math::Vec4(baseColor.r, baseColor.g, baseColor.b, baseColor.a);
    }

    float opacity = 1.0f;
    if (aiMat->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS) {
        matData.baseColorFactor.w *= opacity;
    }

    aiColor3D emissive(0.0f, 0.0f, 0.0f);
    if (aiMat->Get(AI_MATKEY_COLOR_EMISSIVE, emissive) == AI_SUCCESS) {
        matData.emissiveFactor = math::Vec3(emissive.r, emissive.g, emissive.b);
    }

    float metallic = 0.0f;
    if (aiMat->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS) {
        matData.metallicFactor = metallic;
    }

    float roughness = 1.0f;
    if (aiMat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS) {
        matData.roughnessFactor = roughness;
    }

    aiString alphaMode;
    if (aiMat->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == AI_SUCCESS) {
        std::string modeStr(alphaMode.C_Str());
        ENGINE_LOG_INFO("Material alphaMode: '{}'", modeStr);
        if (modeStr == "OPAQUE") {
            matData.alphaCutoff = 0.0f;
        } else if (modeStr == "MASK") {
            float alphaCutoff = 0.5f;
            if (aiMat->Get(AI_MATKEY_GLTF_ALPHACUTOFF, alphaCutoff) == AI_SUCCESS) {
                matData.alphaCutoff = alphaCutoff;
            } else {
                matData.alphaCutoff = 0.5f;
            }
        } else {
            // BLEND mode: discard transparent border pixels so they don't block geometry
            matData.alphaCutoff = 0.001f;
        }
    } else {
        ENGINE_LOG_INFO("Material alphaMode not specified -> OPAQUE");
        matData.alphaCutoff = 0.0f;
    }

    float occlusionStrength = 1.0f;
    if (aiMat->Get(AI_MATKEY_GLTF_TEXTURE_STRENGTH(aiTextureType_AMBIENT_OCCLUSION, 0), occlusionStrength) == AI_SUCCESS ||
        aiMat->Get(AI_MATKEY_GLTF_TEXTURE_STRENGTH(aiTextureType_LIGHTMAP, 0), occlusionStrength) == AI_SUCCESS) {
        matData.occlusionStrength = occlusionStrength;
    }

    // 3. Create Uniform Buffer
    bifrost::BufferDesc bufferDesc{};
    bufferDesc.initData = &matData;
    bufferDesc.size = sizeof(MaterialData);
    bufferDesc.type = bifrost::BufferType::UNIFORM;
    auto materialBuffer = bifrost->createBuffer(bufferDesc);

    // 4. Create BindGroup
    bifrost::BindGroupDesc bindGroupDesc{};
    bindGroupDesc.entries.push_back({0, materialBuffer, nullptr});
    bindGroupDesc.entries.push_back({1, nullptr, baseColorTex ? baseColorTex : DefaultResources::getDefaultTexture()});
    bindGroupDesc.entries.push_back({2, nullptr, normalTex ? normalTex : DefaultResources::getDefaultNormalTexture()});
    bindGroupDesc.entries.push_back({3, nullptr, mrTex ? mrTex : DefaultResources::getDefaultTexture()});
    bindGroupDesc.entries.push_back({4, nullptr, aoTex ? aoTex : DefaultResources::getDefaultTexture()});
    bindGroupDesc.entries.push_back({5, nullptr, emissiveTex ? emissiveTex : DefaultResources::getDefaultBlackTexture()});

    auto bindGroup = bifrost->createBindGroup(bindGroupDesc);

    return std::make_shared<Material>(singlePipe, instancedPipe, bindGroup);
}

std::shared_ptr<scene::SceneNode> processNode(const aiNode* node, const aiScene* scene,
                                              const std::vector<std::shared_ptr<scene::Model>>& loadedModels) {

    auto sceneNode = std::make_shared<scene::SceneNode>();
    sceneNode->setTransform(convertTransform(node->mTransformation));

    // Attach all models for this node's meshes
    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        unsigned int meshIndex = node->mMeshes[i];
        if (meshIndex < loadedModels.size()) {
            sceneNode->addModel(loadedModels[meshIndex]);
        }
    }

    // Recursively process child nodes
    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        sceneNode->addChild(processNode(node->mChildren[i], scene, loadedModels));
    }

    return sceneNode;
}

} // namespace

std::shared_ptr<scene::SceneNode> ModelImporter::loadModel(const std::string& filePath, bifrost::Bifrost* bifrost,
                                                           bifrost::PipelineManager& pipelineManager, const std::string& shaderPath) {

    ENGINE_LOG_INFO("Loading model: {}", filePath);

    std::string shaderKey = std::filesystem::path(shaderPath).stem().string();
    std::string singleKey = shaderKey + "_single";
    std::string instancedKey = shaderKey + "_instanced";

    auto singlePipeline = pipelineManager.getPipeline(singleKey);
    auto instancedPipeline = pipelineManager.getPipeline(instancedKey);

    if (!singlePipeline || !instancedPipeline) {
        // Read shader code with fallback to base.hlsl if specified shader is missing
        const std::string ASSET_FOLDER(ASSETS_PATH);
        std::string shaderCode = utils::file::readTextFile(ASSET_FOLDER + shaderPath);
        std::string effectiveShader = shaderPath;

        if (shaderCode.empty()) {
            ENGINE_LOG_WARN("Failed to read shader at '{}{}', falling back to 'shaders/base.hlsl'...", ASSET_FOLDER, shaderPath);
            shaderCode = utils::file::readTextFile(ASSET_FOLDER + "shaders/base.hlsl");
            effectiveShader = "shaders/base.hlsl";
            if (shaderCode.empty()) {
                ENGINE_LOG_ERROR("Failed to read base shader at {}shaders/base.hlsl", ASSET_FOLDER);
                return nullptr;
            }
        }

        std::string effectiveKey = std::filesystem::path(effectiveShader).stem().string();
        singleKey = effectiveKey + "_single";
        instancedKey = effectiveKey + "_instanced";

        singlePipeline = pipelineManager.getPipeline(singleKey);
        instancedPipeline = pipelineManager.getPipeline(instancedKey);

        if (!singlePipeline || !instancedPipeline) {
            auto fsShared = bifrost->createShader(bifrost::ShaderType::FRAGMENT, shaderCode, false);

            if (!singlePipeline) {
                auto vsSingle = bifrost->createShader(bifrost::ShaderType::VERTEX, shaderCode, false);
                bifrost::PipelineInfo singlePipeInfo{vsSingle, fsShared, false};
                singlePipeline = pipelineManager.getOrCreatePipeline(bifrost, singleKey, singlePipeInfo);
            }

            if (!instancedPipeline) {
                auto vsInstanced = bifrost->createShader(bifrost::ShaderType::VERTEX, shaderCode, true);
                bifrost::PipelineInfo instancedPipeInfo{vsInstanced, fsShared, true};
                instancedPipeline = pipelineManager.getOrCreatePipeline(bifrost, instancedKey, instancedPipeInfo);
            }
        }
    }

    // Import scene with Assimp
    Assimp::Importer importer;
    constexpr unsigned int importFlags =
        aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType | aiProcess_FlipUVs;

    const aiScene* scene = importer.ReadFile(filePath, importFlags);

    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
        ENGINE_LOG_ERROR("Failed to import model '{}': {}", filePath, importer.GetErrorString());
        return nullptr;
    }

    const std::filesystem::path modelPath(filePath);
    const std::string directory = modelPath.parent_path().string();

    std::unordered_map<std::string, std::shared_ptr<bifrost::Texture>> textureCache;

    // Process materials
    std::vector<std::shared_ptr<Material>> loadedMaterials;
    loadedMaterials.reserve(scene->mNumMaterials);

    for (unsigned int matIndex = 0; matIndex < scene->mNumMaterials; ++matIndex) {
        const aiMaterial* aiMat = scene->mMaterials[matIndex];
        loadedMaterials.push_back(processMaterial(aiMat, scene, directory, bifrost, singlePipeline, instancedPipeline, textureCache));
    }

    // Fallback material if scene has no materials
    MaterialData fallbackData{};
    bifrost::BufferDesc fallbackBufferDesc{};
    fallbackBufferDesc.initData = &fallbackData;
    fallbackBufferDesc.size = sizeof(MaterialData);
    fallbackBufferDesc.type = bifrost::BufferType::UNIFORM;
    auto fallbackBuffer = bifrost->createBuffer(fallbackBufferDesc);

    bifrost::BindGroupDesc fallbackBindGroupDesc{};
    fallbackBindGroupDesc.entries.push_back({0, fallbackBuffer, nullptr});
    fallbackBindGroupDesc.entries.push_back({1, nullptr, DefaultResources::getDefaultTexture()});
    fallbackBindGroupDesc.entries.push_back({2, nullptr, DefaultResources::getDefaultNormalTexture()});
    fallbackBindGroupDesc.entries.push_back({3, nullptr, DefaultResources::getDefaultTexture()});
    fallbackBindGroupDesc.entries.push_back({4, nullptr, DefaultResources::getDefaultTexture()});
    fallbackBindGroupDesc.entries.push_back({5, nullptr, DefaultResources::getDefaultBlackTexture()});
    auto fallbackBindGroup = bifrost->createBindGroup(fallbackBindGroupDesc);

    auto fallbackMaterial = std::make_shared<Material>(singlePipeline, instancedPipeline, fallbackBindGroup);

    // Process meshes and pair them with materials to create Model instances
    std::vector<std::shared_ptr<scene::Model>> loadedModels;
    loadedModels.reserve(scene->mNumMeshes);

    for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
        const aiMesh* aiMesh = scene->mMeshes[meshIndex];
        auto mesh = processMesh(aiMesh, bifrost);

        std::shared_ptr<Material> material = fallbackMaterial;
        if (aiMesh->mMaterialIndex < loadedMaterials.size()) {
            material = loadedMaterials[aiMesh->mMaterialIndex];
        }

        ENGINE_LOG_INFO("Mesh {} '{}': matIndex={}, numVertices={}, numFaces={}",
                        meshIndex,
                        aiMesh->mName.C_Str(),
                        aiMesh->mMaterialIndex,
                        aiMesh->mNumVertices,
                        aiMesh->mNumFaces);
        loadedModels.push_back(std::make_shared<scene::Model>(mesh, material));
    }

    // Build the scene graph hierarchy
    auto rootNode = processNode(scene->mRootNode, scene, loadedModels);

    ENGINE_LOG_INFO("Model loaded successfully: {} ({} meshes, {} materials)", filePath, scene->mNumMeshes, scene->mNumMaterials);

    return rootNode;
}

} // namespace midgard::resource
