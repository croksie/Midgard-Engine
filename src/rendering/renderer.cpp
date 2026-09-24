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
#include "scene/scene.h"

namespace midgard::render
{

std::string assetFolder(ASSETS_PATH);

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

    // Init SceneGraph
    
    m_sceneGraph = std::make_shared<midgard::scene::SceneGraph>();

    std::vector<scene::Model> importedModels = utils::importer::importModel(assetFolder + "model/suzanne.glb", m_rhi.get(), false);
    for(scene::Model importedModel : importedModels){
        m_sceneGraph->addModel(importedModel);
    }

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


    const std::vector<scene::Model>& models = m_sceneGraph->getModels();

    ENGINE_LOG_TRACE("Render start");
    m_rhi->beginFrame();
    m_rhi->clear();
    m_rhi->setGlobalUniform(data, sizeof(matrices));

    // Render unique models
    for (auto &model : models) {
        std::shared_ptr<resource::Mesh> mesh = model.getMesh();
        std::shared_ptr<resource::Material> material = model.getMaterial();

        math::Mat4 modelMat = model.getTransform();
        m_rhi->bindPipeline(material->getPipeline().get());
        m_rhi->bindVertexBuffer(material->getPipeline(), mesh->getVertexBuffer());
        m_rhi->bindIndexBuffer(material->getPipeline(), mesh->getIndexBuffer());
        m_rhi->setLocalUniform(&modelMat, sizeof(math::Mat4));
        m_rhi->bindTexture(material->getPipeline(), material->getTexture(), 0);
        m_rhi->draw(material->getPipeline());
    }

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

    m_rhi->endFrame();
    ENGINE_LOG_TRACE("Render end");
}

void Renderer::shutdown() {
    ENGINE_LOG_DEBUG("Renderer shutting down ...");
    m_sceneGraph = nullptr;
    resource::DefaultResources::shutdown();
    m_rhi->shutdown(); // Ensure to have free all buffer and pipeline before
}

} // namespace midgard::render
