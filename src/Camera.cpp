#include "Camera.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

Camera::Camera(glm::vec3 position, glm::vec3 target, glm::vec3 up)
    : m_position(position)
    , m_target(target)
    , m_worldUp(up)
    , m_orientation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f))
    , m_movementSpeed(2.5f)
    , m_rotationSpeed(1.0f)
    , m_fov(45.0f)
    , m_nearPlane(0.1f)
    , m_farPlane(100.0f)
{
    m_initialPosition = position;
    m_initialTarget = target;
    m_initialOrientation = m_orientation;
    updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() const
{
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), -m_target);
    glm::mat4 rotation = glm::toMat4(m_orientation);
    glm::mat4 positionTranslation = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -getDistance()));

    return positionTranslation * rotation * translation;
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio) const
{
    return glm::perspective(glm::radians(m_fov), aspectRatio, m_nearPlane, m_farPlane);
}

void Camera::processKeyboard(CameraMovement direction, float deltaTime)
{
    float velocity = m_movementSpeed * deltaTime;
    glm::vec3 forward = glm::normalize(m_target - m_position);
    glm::vec3 right = glm::normalize(glm::cross(forward, m_worldUp));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));

    switch (direction)
    {
        case CameraMovement::FORWARD:
            m_position += forward * velocity;
            m_target += forward * velocity;
            break;
        case CameraMovement::BACKWARD:
            m_position -= forward * velocity;
            m_target -= forward * velocity;
            break;
        case CameraMovement::LEFT:
            m_position -= right * velocity;
            m_target -= right * velocity;
            break;
        case CameraMovement::RIGHT:
            m_position += right * velocity;
            m_target += right * velocity;
            break;
        case CameraMovement::UP:
            m_position += up * velocity;
            m_target += up * velocity;
            break;
        case CameraMovement::DOWN:
            m_position -= up * velocity;
            m_target -= up * velocity;
            break;
    }
}

void Camera::processTrackball(const glm::vec2& prevPos, const glm::vec2& currPos, const glm::vec2& screenSize)
{
    if (glm::length(currPos - prevPos) < 0.001f)
        return;

    glm::vec3 va = mapToSphere(prevPos, screenSize);
    glm::vec3 vb = mapToSphere(currPos, screenSize);

    float angle = glm::acos(glm::clamp(glm::dot(va, vb), -1.0f, 1.0f));
    glm::vec3 axis = glm::cross(va, vb);

    if (glm::length(axis) > 0.001f)
    {
        axis = glm::normalize(axis);
        glm::quat rotation = glm::angleAxis(angle * m_rotationSpeed, axis);
        m_orientation = rotation * m_orientation;
        m_orientation = glm::normalize(m_orientation);

        updateCameraVectors();
    }
}

void Camera::processMouseScroll(float yoffset)
{
    float distance = getDistance();
    distance -= yoffset * 0.1f;
    distance = glm::clamp(distance, 0.5f, 50.0f);

    glm::vec3 direction = glm::normalize(m_position - m_target);
    m_position = m_target + direction * distance;
}

void Camera::reset()
{
    m_position = m_initialPosition;
    m_target = m_initialTarget;
    m_orientation = m_initialOrientation;
    updateCameraVectors();
}

void Camera::updateCameraVectors()
{
    glm::mat4 rotationMatrix = glm::toMat4(m_orientation);

    m_right = glm::normalize(glm::vec3(rotationMatrix[0]));
    m_up = glm::normalize(glm::vec3(rotationMatrix[1]));

    glm::vec3 direction = glm::normalize(m_position - m_target);
    m_position = m_target + direction * getDistance();
}

glm::vec3 Camera::mapToSphere(const glm::vec2& point, const glm::vec2& screenSize) const
{
    glm::vec2 normalizedPoint;
    normalizedPoint.x = (2.0f * point.x - screenSize.x) / screenSize.x;
    normalizedPoint.y = (screenSize.y - 2.0f * point.y) / screenSize.y;

    float lengthSquared = normalizedPoint.x * normalizedPoint.x + normalizedPoint.y * normalizedPoint.y;

    glm::vec3 result;
    if (lengthSquared <= 0.5f)
    {
        result.z = glm::sqrt(1.0f - lengthSquared);
    }
    else
    {
        result.z = 0.5f / glm::sqrt(lengthSquared);
    }

    result.x = normalizedPoint.x;
    result.y = normalizedPoint.y;

    return glm::normalize(result);
}
