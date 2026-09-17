#include "rendering/renderer.h"

#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "math/math.h"
#include "resources/default.h"
#include "utils/file_utils.h"
#include "utils/importer.h"
#include "utils/log.h"

#include "bifrost/buffer.h"
#include "bifrost/opengl/opengl_bifrost.h"
#include "bifrost/pipeline.h"
#include "bifrost/shader.h"
#include "bifrost/texture.h"
#include "bifrost/vulkan/vulkan_bifrost.h"
#include "core/config.h"
#include "platform/input/input.h"
#include "platform/window/window.h"
#include "resources/material.h"
#include "resources/mesh.h"

namespace midgard::render
{

std::string assetFolder(ASSETS_PATH);

float vertices[] = {
    // Face Front
    -0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, -0.5f, 0.5f, 1.0f,
    0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
    -0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    // Face Back
    0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -0.5f, -0.5f, -0.5f, 0.0f,
    1.0f, 0.0f, 1.0f, 0.0f, -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    // Face Left
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -0.5f, -0.5f, 0.5f, 0.0f,
    0.0f, 1.0f, 1.0f, 0.0f, -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
    // Face Right
    0.5f, -0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.5f, -0.5f, -0.5f, 1.0f,
    1.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    // Face Up
    -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f, 0.5f, 0.0f,
    1.0f, 1.0f, 1.0f, 0.0f, 0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
    // Face Down
    -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, -0.5f, -0.5f, 1.0f,
    0.0f, 1.0f, 1.0f, 0.0f, 0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    -0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f};

uint32_t indices[] = {
    // Front
    0, 1, 2, 2, 3, 0,
    // Back
    4, 5, 6, 6, 7, 4,
    // Left
    8, 9, 10, 10, 11, 8,
    // Right
    12, 13, 14, 14, 15, 12,
    // Up
    16, 17, 18, 18, 19, 16,
    // Down
    20, 21, 22, 22, 23, 20};

void Renderer::createTestModel() {
    std::string shaderSource = utils::file::readTextFile(assetFolder + "shaders/base.hlsl");

    // Create texture
    int width = 0, height = 0, nrChannels = 0;
    unsigned char *data = stbi_load((assetFolder + "textures/wall.jpg").c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);
    if (!data) {
        ENGINE_LOG_ERROR("Renderer::createTestModel: Failed to load texture '{}'", assetFolder + "textures/wall.jpg");
    }
    else {
        ENGINE_LOG_INFO("Renderer::createTestModel: Loaded texture ({}x{}, channels: {})", width, height, nrChannels);
    }

    bifrost::TextureDesc texDesc{};
    texDesc.width = static_cast<uint32_t>(width);
    texDesc.height = static_cast<uint32_t>(height);
    texDesc.format = bifrost::TextureFormat::RGBA8_SRGB;
    texDesc.initialData = data;
    texDesc.size = width * height * 4;

    auto texture = m_rhi->createTexture(texDesc);

    if (data) {
        stbi_image_free(data);
    }

    // Create pipeline
    bifrost::PipelineInfo info;
    info.vertexShader = m_rhi->createShader(bifrost::ShaderType::VERTEX, shaderSource, true);
    info.fragmentShader = m_rhi->createShader(bifrost::ShaderType::FRAGMENT, shaderSource, false);
    info.useInstance = true;

    auto pipeline = m_rhi->createPipeline(info);

    // Create Material
    resource::Material material(pipeline, texture);

    // Create Mesh
    bifrost::BufferDesc vertexBufferDesc{};
    vertexBufferDesc.initData = vertices;
    vertexBufferDesc.size = sizeof(vertices);
    vertexBufferDesc.type = bifrost::BufferType::VERTEX;

    auto vertexBuffer = m_rhi->createBuffer(vertexBufferDesc);

    bifrost::BufferDesc indexBufferDesc{};
    indexBufferDesc.initData = indices;
    indexBufferDesc.size = sizeof(indices);
    indexBufferDesc.type = bifrost::BufferType::INDEX;

    auto indexBuffer = m_rhi->createBuffer(indexBufferDesc);

    resource::Mesh mesh(vertexBuffer, indexBuffer);

    // Create Models
    for (int i = -15; i < 15; ++i) {
        for (int j = -15; j < 15; ++j) {
            scene::Model model(std::make_shared<resource::Mesh>(mesh), std::make_shared<resource::Material>(material));
            model.setPosition(math::Vec3(2.0f * i, 2.0f * j, -15.0f));
            models.push_back(model);
        }
    }

    const float time = static_cast<float>(glfwGetTime());
    const glm::mat4 baseRotation = glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-55.0f), 
                                    glm::vec3(1.0f, 0.0f, 0.0f)), time * glm::radians(50.0f), glm::vec3(0.5f, 1.0f, 0.0f));

    for (size_t i = 0; i < models.size(); ++i) {
        glm::mat4 modelMat = baseRotation;
        modelMat[3] = glm::vec4(models[i].getPosition().toGlm(), 1.0f);
        m_instances[i].modelMatrix = math::Mat4(modelMat);
    }

    bifrost::BufferDesc desc{};
    desc.size = m_instances.size() * sizeof(bifrost::InstanceData);
    desc.type = bifrost::BufferType::VERTEX;
    desc.initData = m_instances.data();

    m_instanceBuffer = m_rhi->createBuffer(desc);
}

void Renderer::initialize(platform::Window *window, std::shared_ptr<core::EngineConfig> config) {
    ENGINE_LOG_INFO("Renderer initializing ...");
    m_config = config;

    // Init Bifrost
    switch (m_config->api) {
    case core::GraphicsAPI::OpenGL:
        m_rhi = std::make_unique<bifrost::opengl::OpenGLBifrost>();
        break;
    case core::GraphicsAPI::Vulkan:
        m_rhi = std::make_unique<bifrost::vulkan::VulkanBifrost>();
        break;
    }
    m_rhi->initialize(window, m_config);

    // Init default Resources
    resource::DefaultResources::init(m_rhi.get());

    // Init Camera
    core::CameraConfig cameraConfig{};
    m_camera = scene::Camera(cameraConfig);

    // Create Test Models
    //createTestModel();
    m_model = utils::importer::importModel(assetFolder + "model/suzanne.glb", m_rhi.get(), false);

    ENGINE_LOG_INFO("Renderer initialized");
}

void Renderer::render() {
    // Delta time calculation
    const float currentTime = static_cast<float>(glfwGetTime());
    const float deltaTime = (m_lastFrameTime > 0.0f) ? (currentTime - m_lastFrameTime) : 0.016f;
    m_lastFrameTime = currentTime;

    // Mouse Look
    double mouseX, mouseY;
    platform::Input::getMousePosition(mouseX, mouseY);
    if (m_firstMouse) {
        m_lastMouseX = mouseX;
        m_lastMouseY = mouseY;
        m_firstMouse = false;
    }
    float xOffset = static_cast<float>(mouseX - m_lastMouseX);
    float yOffset = static_cast<float>(m_lastMouseY - mouseY);
    m_lastMouseX = mouseX;
    m_lastMouseY = mouseY;

    m_camera.processMouseMovement(xOffset, yOffset);

    // Keyboard Movement
    if (platform::Input::isKeyPressed(platform::KeyCode::Z))
        m_camera.processKeyboard(m_camera.getFront(), deltaTime);
    if (platform::Input::isKeyPressed(platform::KeyCode::S))
        m_camera.processKeyboard(-m_camera.getFront(), deltaTime);
    if (platform::Input::isKeyPressed(platform::KeyCode::Q))
        m_camera.processKeyboard(-m_camera.getRight(), deltaTime);
    if (platform::Input::isKeyPressed(platform::KeyCode::D))
        m_camera.processKeyboard(m_camera.getRight(), deltaTime);
    if (platform::Input::isKeyPressed(platform::KeyCode::Space))
        m_camera.processKeyboard(math::Vec3(0.0f, 1.0f, 0.0f), deltaTime);

    glm::mat4 matrices[2] = {m_camera.getViewMatrix(), m_camera.getProjectionMatrix()};
    const void *data = matrices;

    ENGINE_LOG_TRACE("Render start");
    m_rhi->beginFrame();
    m_rhi->clear();
    m_rhi->setGlobalUniform(data, sizeof(matrices));

    // const float time = static_cast<float>(glfwGetTime());
    // const glm::mat4 baseRotation =
    //     glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-55.0f),
    //                             glm::vec3(1.0f, 0.0f, 0.0f)),
    //                 time * glm::radians(50.0f), glm::vec3(0.5f, 1.0f, 0.0f));

    // for (size_t i = 0; i < models.size(); ++i) {
    //     glm::mat4 modelMat = baseRotation;
    //     modelMat[3] = glm::vec4(models[i].getPosition().toGlm(), 1.0f);
    //     m_instances[i].modelMatrix = math::Mat4(modelMat);
    // }

    // m_instanceBuffer->setData(m_instances.size() * sizeof(bifrost::InstanceData), m_instances.data());

    // for (auto &model : models) {
    // std::shared_ptr<resource::Mesh> mesh = models[0].getMesh();
    // std::shared_ptr<resource::Material> material = models[0].getMaterial();

    // m_rhi->bindPipeline(material->getPipeline().get());
    // m_rhi->bindVertexBuffer(material->getPipeline(), mesh->getVertexBuffer());
    // m_rhi->bindIndexBuffer(material->getPipeline(), mesh->getIndexBuffer());
    // m_rhi->bindInstanceBuffer(material->getPipeline(), m_instanceBuffer);
    // m_rhi->bindTexture(material->getPipeline(), material->getTexture(), 0);
    // m_rhi->draw(material->getPipeline(), 900);
    //}

    math::Mat4 modelMat = math::Mat4(1.0f);
    auto material = m_model.getMaterial();
    auto mesh = m_model.getMesh();
    m_rhi->bindPipeline(material->getPipeline().get());
    m_rhi->bindVertexBuffer(material->getPipeline(), mesh->getVertexBuffer());
    m_rhi->bindIndexBuffer(material->getPipeline(), mesh->getIndexBuffer());
    m_rhi->setLocalUniform(&modelMat, sizeof(modelMat));
    m_rhi->bindTexture(material->getPipeline(), material->getTexture(), 0);
    m_rhi->draw(material->getPipeline(), 1);

    m_rhi->endFrame();
    ENGINE_LOG_TRACE("Render end");
}

void Renderer::shutdown() {
    ENGINE_LOG_DEBUG("Renderer shutting down ...");
    models.clear();
    m_instanceBuffer = nullptr;
    m_model = scene::Model();
    resource::DefaultResources::shutdown();
    m_rhi->shutdown(); // Ensure to have free all buffer and pipeline before
}

} // namespace midgard::render
