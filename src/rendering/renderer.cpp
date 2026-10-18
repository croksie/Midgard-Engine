#include "rendering/renderer.h"

#include <memory>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "math/math.h"
#include "utils/file_utils.h"
#include "utils/importer.h"
#include "utils/log.h"

#include "bifrost/buffer.h"
#include "bifrost/opengl/opengl_bifrost.h"
#include "bifrost/pipeline.h"
#include "bifrost/pipeline_manager.h"
#include "bifrost/shader.h"
#include "bifrost/texture.h"
#include "bifrost/vulkan/vulkan_bifrost.h"

#include "core/config.h"
#include "platform/input/input.h"
#include "platform/window/window.h"
#include "rendering/render_packet.h"
#include "rendering/render_scene.h"
#include "resources/default.h"
#include "resources/material.h"
#include "resources/mesh.h"
#include "resources/model_importer.h"
#include "scene/scene.h"

namespace midgard::render {

std::string assetFolder(ASSETS_PATH);

void Renderer::initialize(platform::Window* window, std::shared_ptr<core::EngineConfig> config) {
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

    // Init instance buffer
    bifrost::BufferDesc instanceBufDesc{};
    instanceBufDesc.type = bifrost::BufferType::VERTEX;
    instanceBufDesc.size = MAX_INSTANCES * sizeof(bifrost::InstanceData);
    instanceBufDesc.initData = nullptr;
    m_instanceBuffer = m_rhi->createBuffer(instanceBufDesc);

    // Init default Resources
    resource::DefaultResources::init(m_rhi.get());

    // Init Camera
    core::CameraConfig cameraConfig{};
    m_camera = scene::Camera(cameraConfig);

    // Init SceneGraph
    m_sceneGraph = std::make_shared<midgard::scene::SceneGraph>();

    bifrost::PipelineManager pipelineManager;

    auto rootNode = std::make_unique<scene::SceneNode>();
    auto modelNode = resource::ModelImporter::loadModel(assetFolder + "model/voiture.glb", m_rhi.get(), pipelineManager);
    if (modelNode) {
        rootNode->addChild(modelNode);
    }
    m_sceneGraph->setRootNode(std::move(rootNode));

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

    // World martrix
    glm::mat4 matrices[2] = {m_camera.getViewMatrix(), m_camera.getProjectionMatrix()};
    const void* data = matrices;

    renderer::RenderScene renderScene = m_sceneGraph->generateRenderScene();

    ENGINE_LOG_TRACE("Render start");
    m_rhi->beginFrame();
    m_rhi->clear();
    m_rhi->setGlobalUniform(data, sizeof(matrices));

    // Render unique models
    for (const auto& packet : renderScene.m_singleDraws) {
        auto pipeline = packet.material->getPipeline(false);
        m_rhi->bindPipeline(pipeline.get());
        m_rhi->bindVertexBuffer(pipeline, packet.mesh->getVertexBuffer());
        m_rhi->bindIndexBuffer(pipeline, packet.mesh->getIndexBuffer());
        m_rhi->setLocalUniform(&packet.worldTransform, sizeof(math::Mat4));
        m_rhi->bindTexture(pipeline, packet.material->getTexture(), 0);
        m_rhi->draw(pipeline, 1);
    }

    // Render instanced models
    size_t currentInstanceOffset = 0;
    for (const auto& packet : renderScene.m_intancedDraws) {
        size_t dataSize = packet.instanceTransforms.size() * sizeof(math::Mat4);

        m_instanceBuffer->setData(dataSize, packet.instanceTransforms.data(), currentInstanceOffset);

        auto pipeline = packet.material->getPipeline(true);
        m_rhi->bindPipeline(pipeline.get());
        m_rhi->bindVertexBuffer(pipeline, packet.mesh->getVertexBuffer());
        m_rhi->bindInstanceBuffer(pipeline, m_instanceBuffer, static_cast<uint32_t>(currentInstanceOffset));
        m_rhi->bindIndexBuffer(pipeline, packet.mesh->getIndexBuffer());
        m_rhi->bindTexture(pipeline, packet.material->getTexture(), 0);
        m_rhi->draw(pipeline, static_cast<uint32_t>(packet.instanceTransforms.size()));
    }

    m_rhi->endFrame();
    ENGINE_LOG_TRACE("Render end");
}

void Renderer::shutdown() {
    ENGINE_LOG_DEBUG("Renderer shutting down ...");
    m_sceneGraph = nullptr;
    m_instanceBuffer = nullptr;
    resource::DefaultResources::shutdown();
    m_rhi->shutdown(); // Ensure to have free all buffer and pipeline before
}

} // namespace midgard::render
