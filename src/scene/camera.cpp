#include "scene/camera.h"

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace midgard::scene {

Camera::Camera(const core::CameraConfig& config, math::Vec3 position) : m_config(config), m_position(position) {
    m_projection = glm::perspectiveRH_ZO(glm::radians(config.fov), config.widthResolution / config.heightResolution, 0.1f, 100.0f);
    updateCameraVectors();
}

void Camera::updateProjection(float width, float height) {
    m_config.widthResolution = width;
    m_config.heightResolution = height;
    if (height > 0.0f) {
        m_projection = glm::perspectiveRH_ZO(glm::radians(m_config.fov), width / height, 0.1f, 100.0f);
    }
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt<float, glm::defaultp>(m_position.toGlm(), (m_position + m_front).toGlm(), m_up.toGlm());
}

void Camera::setPosition(const math::Vec3& newPosition) {
    m_position = newPosition;
}

void Camera::processMouseMovement(float xOffset, float yOffset, bool constrainPitch) {
    xOffset *= m_sensitivity;
    yOffset *= m_sensitivity;

    m_yaw   += xOffset;
    m_pitch += yOffset;

    if (constrainPitch) {
        m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
    }

    updateCameraVectors();
}

void Camera::processKeyboard(const math::Vec3& direction, float deltaTime) {
    float velocity = m_speed * deltaTime;
    m_position += direction * velocity;
}

void Camera::updateCameraVectors() {
    glm::vec3 front;
    front.x = std::cos(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
    front.y = std::sin(glm::radians(m_pitch));
    front.z = std::sin(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
    glm::vec3 normFront = glm::normalize(front);
    m_front = math::Vec3(normFront.x, normFront.y, normFront.z);

    glm::vec3 worldUp = m_worldUp.toGlm();
    glm::vec3 right = glm::normalize(glm::cross(normFront, worldUp));
    m_right = math::Vec3(right.x, right.y, right.z);

    glm::vec3 up = glm::normalize(glm::cross(right, normFront));
    m_up = math::Vec3(up.x, up.y, up.z);
}

} // namespace midgard::scene
