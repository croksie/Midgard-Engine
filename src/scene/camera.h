#pragma once
#include "core/config.h"
#include "math/math.h"

namespace midgard::scene {

class Camera {
public:
    Camera() = default;
    Camera(const core::CameraConfig& config, math::Vec3 position = math::Vec3(0.0f, 2.0f, -10.0f));

    glm::mat4 getProjectionMatrix() const { return m_projection; }
    glm::mat4 getViewMatrix() const;
    void updateProjection(float width, float height);

    void processMouseMovement(float xOffset, float yOffset, bool constrainPitch = true);
    void processKeyboard(const math::Vec3& direction, float deltaTime);

    math::Vec3 getPosition() const { return m_position; }
    void setPosition(const math::Vec3& newPosition);

    math::Vec3 getFront() const { return m_front; }
    math::Vec3 getRight() const { return m_right; }
    math::Vec3 getUp() const { return m_up; }

    float getSpeed() const { return m_speed; }
    void setSpeed(float speed) { m_speed = speed; }

    float getSensitivity() const { return m_sensitivity; }
    void setSensitivity(float sensitivity) { m_sensitivity = sensitivity; }

private:
    void updateCameraVectors();

    core::CameraConfig m_config;

    math::Vec3 m_position = math::Vec3(0.0f, 2.0f, -10.0f);
    math::Vec3 m_front = math::Vec3(0.0f, 0.0f, 1.0f);
    math::Vec3 m_up = math::Vec3(0.0f, 1.0f, 0.0f);
    math::Vec3 m_right = math::Vec3(1.0f, 0.0f, 0.0f);
    math::Vec3 m_worldUp = math::Vec3(0.0f, 1.0f, 0.0f);

    glm::mat4 m_projection = glm::mat4(1.0f);

    float m_yaw = 90.0f;
    float m_pitch = 0.0f;
    float m_speed = 6.0f;
    float m_sensitivity = 0.1f;
};

} // namespace midgard::scene