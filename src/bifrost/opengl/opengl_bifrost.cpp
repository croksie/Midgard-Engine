#include "bifrost/opengl/opengl_bifrost.h"

#include <stdexcept>

#include <glad/glad.h>

#include "utils/log.h"
#include "core/config.h"
#include "platform/window/window.h"

#include "bifrost/opengl/opengl_pipeline.h"
#include "bifrost/opengl/opengl_buffer.h"
#include "bifrost/opengl/opengl_shader.h"
#include "bifrost/opengl/opengl_texture.h"

namespace midgard::bifrost::opengl {

void OpenGLBifrost::initialize(platform::Window* window, std::shared_ptr<core::EngineConfig> config) {
    m_window = window;
    m_config = config;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, m_config->resizable);
    // TODO : implement fullscreen mode
    #ifdef DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    #endif

    m_window->initializeWindow(m_config->windowWidth, m_config->windowHeight, m_config->windowTitle.c_str());
    glfwMakeContextCurrent(static_cast<GLFWwindow*>(m_window->getNativeHandle()));
    
    m_window->setWindowSizeCallback([this](int width, int height) {
        this->onWindowResize(width, height);
    });

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        ENGINE_LOG_CRITICAL("Failed to initialize OpenGL context");
    }
    
    ENGINE_LOG_INFO("OpenGL Version {}", (const char*)glGetString(GL_VERSION));
    ENGINE_LOG_INFO("GPU: {}", (const char*)glGetString(GL_RENDERER));

    #ifdef DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
        (void)source;
        (void)type;
        (void)id;
        (void)length;
        (void)userParam;
        if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;
        ENGINE_LOG_WARN("Debug: {}", message);
    }, nullptr);
    #endif

    glfwSwapInterval(m_config->vsync ? 1 : 0);

    glViewport(0, 0, m_config->windowWidth, m_config->windowHeight);
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, m_config->windowWidth, m_config->windowHeight);
    
    // DepthBuffer
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);

    // Backface-Culling
    glEnable(GL_CULL_FACE);  
    glCullFace(GL_BACK);  
    glFrontFace(GL_CCW);

    BufferDesc desc{};
    desc.size = 2048;
    desc.type = BufferType::UNIFORM;
    m_uniformBuffer = std::make_unique<OpenGLBuffer>(desc); // FIXME: Change the buffer size

    ENGINE_LOG_INFO("Initialization success");
}

void OpenGLBifrost::onWindowResize(int width, int height) {
    m_config->windowWidth = width;
    m_config->windowHeight = height;

    glViewport(0, 0, width, height);
    glScissor(0, 0, width, height);
}

void OpenGLBifrost::beginFrame() {
    ENGINE_LOG_TRACE("BeginFrame");
}

void OpenGLBifrost::endFrame() {
    glfwSwapBuffers(static_cast<GLFWwindow*>(m_window->getNativeHandle()));
    glfwPollEvents();
    ENGINE_LOG_TRACE("Buffer swapped");
}

void OpenGLBifrost::clear() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ENGINE_LOG_TRACE("Buffer cleared");
}

std::shared_ptr<Buffer> OpenGLBifrost::createBuffer(BufferDesc& desc) {
    ENGINE_LOG_TRACE("Creating buffer...");
    return std::make_shared<OpenGLBuffer>(desc);
}

void OpenGLBifrost::bindVertexBuffer(std::shared_ptr<Pipeline> pipeline, std::shared_ptr<Buffer> buffer) {
    if (pipeline == nullptr) {
        ENGINE_LOG_CRITICAL("Pipeline is null");
    }
    if (buffer == nullptr) {
        ENGINE_LOG_CRITICAL("Buffer is null");
    }
    pipeline->bindVertexBuffer(buffer);
    ENGINE_LOG_TRACE("Vertex buffer bound");
}

void OpenGLBifrost::bindInstanceBuffer(std::shared_ptr<Pipeline> pipeline, std::shared_ptr<Buffer> buffer) {
    if (pipeline == nullptr) {
        ENGINE_LOG_CRITICAL("Pipeline is null");
    }
    if (buffer == nullptr) {
        ENGINE_LOG_CRITICAL("Buffer is null");
    }
    pipeline->bindInstanceBuffer(buffer);
    ENGINE_LOG_TRACE("Instance buffer bound");
}

void OpenGLBifrost::bindIndexBuffer(std::shared_ptr<Pipeline> pipeline, std::shared_ptr<Buffer> buffer) {
    if (pipeline == nullptr) {
        ENGINE_LOG_CRITICAL("Pipeline is null");
    }
    if (buffer == nullptr) {
        ENGINE_LOG_CRITICAL("Buffer is null");
    }
    pipeline->bindIndexBuffer(buffer);
    ENGINE_LOG_TRACE("Index buffer bound");
}

std::shared_ptr<Shader> OpenGLBifrost::createShader(ShaderType type, std::string source, bool isInstanced) {
    std::string entryPoint = "main";
    switch (type) {
    case ShaderType::VERTEX:
        entryPoint = isInstanced ? "VSMainInstanced" : "VSMain";
        break;
    case ShaderType::FRAGMENT:
        entryPoint = "PSMain";
        break;
    }
    return std::make_shared<OpenGLShader>(type, source, entryPoint);
}

std::shared_ptr<Pipeline> OpenGLBifrost::createPipeline(PipelineInfo& info) {
    ENGINE_LOG_TRACE("Creating pipeline...");
    return std::make_shared<OpenGLPipeline>(info);
}

void OpenGLBifrost::bindPipeline(Pipeline* pipeline) {
    if (pipeline == nullptr) {
        ENGINE_LOG_CRITICAL("Not a valid pipeline");
    }
    ENGINE_LOG_TRACE("Binding pipeline...");
    GLuint shaderProgram = static_cast<OpenGLPipeline*>(pipeline)->getShaderProgramID();
    glUseProgram(shaderProgram); 
    ENGINE_LOG_TRACE("Pipeline bound");
}

std::shared_ptr<Texture> OpenGLBifrost::createTexture(const TextureDesc& desc) {
    ENGINE_LOG_TRACE("Creating texture...");
    return std::make_shared<OpenGLTexture>(desc);
}

void OpenGLBifrost::bindTexture(std::shared_ptr<Pipeline> pipeline, std::shared_ptr<Texture> texture, uint32_t slot) {
    if (pipeline == nullptr) {
        ENGINE_LOG_CRITICAL("Pipeline is null");
    }
    if (texture == nullptr) {
        ENGINE_LOG_CRITICAL("Texture is null");
    }
    pipeline->bindTexture(texture, slot);
    ENGINE_LOG_TRACE("Texture bound");
}

void OpenGLBifrost::setGlobalUniform(const void *data, size_t size) {
    m_uniformBuffer->setData(size, data, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_uniformBuffer->getBufferId());
}

void OpenGLBifrost::setLocalUniform(const void *data, size_t size) {
    (void)size;
    GLint currentProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    if (currentProgram > 0) {
        GLint loc = glGetUniformLocation(currentProgram, "u_Local.model");
        if (loc == -1) {
            loc = 0;
        }
        glUniformMatrix4fv(loc, 1, GL_FALSE, reinterpret_cast<const GLfloat*>(data));
    } else {
        glUniformMatrix4fv(0, 1, GL_FALSE, reinterpret_cast<const GLfloat*>(data));
    }
}

void OpenGLBifrost::draw(std::shared_ptr<Pipeline> pipeline, uint32_t count) {
    ENGINE_LOG_TRACE("Drawing...");
    OpenGLPipeline* openGlPipeline = static_cast<OpenGLPipeline*>(pipeline.get());
    
    glBindVertexArray(openGlPipeline->getVertexArrayID());
    glDrawElementsInstanced(GL_TRIANGLES, openGlPipeline->getBindedNumberOfIndices(), GL_UNSIGNED_INT, nullptr, count);
    ENGINE_LOG_TRACE("Drawn");
}

} // namespace midgard::bifrost::opengl
