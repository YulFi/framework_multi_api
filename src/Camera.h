#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

enum class CameraMovement
{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

class Camera
{
public:
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f),
           glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f),
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;

    void processKeyboard(CameraMovement direction, float deltaTime);
    void processTrackball(const glm::vec2& prevPos, const glm::vec2& currPos, const glm::vec2& screenSize);
    void processMouseScroll(float yoffset);

    glm::vec3 getPosition() const { return m_position; }
    glm::vec3 getTarget() const { return m_target; }
    glm::vec3 getUp() const { return m_up; }
    glm::vec3 getRight() const { return m_right; }
    glm::quat getOrientation() const { return m_orientation; }

    void setPosition(const glm::vec3& position) { m_position = position; updateCameraVectors(); }
    void setTarget(const glm::vec3& target) { m_target = target; updateCameraVectors(); }
    void setMovementSpeed(float speed) { m_movementSpeed = speed; }
    void setRotationSpeed(float speed) { m_rotationSpeed = speed; }
    void setFov(float fov) { m_fov = fov; }
    void setNearPlane(float nearPlane) { m_nearPlane = nearPlane; }
    void setFarPlane(float farPlane) { m_farPlane = farPlane; }

    void reset();

    float getFov() const { return m_fov; }
    float getDistance() const { return glm::length(m_position - m_target); }

private:
    void updateCameraVectors();
    glm::vec3 mapToSphere(const glm::vec2& point, const glm::vec2& screenSize) const;

    glm::vec3 m_position;
    glm::vec3 m_target;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_worldUp;

    glm::quat m_orientation;

    glm::vec3 m_initialPosition;
    glm::vec3 m_initialTarget;
    glm::quat m_initialOrientation;

    float m_movementSpeed;
    float m_rotationSpeed;
    float m_fov;
    float m_nearPlane;
    float m_farPlane;
};
