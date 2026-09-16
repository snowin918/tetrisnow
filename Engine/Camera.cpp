#include "Engine/Camera.h"

#include <glm/gtc/matrix_transform.hpp>

void Camera::setViewportSize(int widthPx, int heightPx)
{
    m_viewportWidth = widthPx > 0 ? widthPx : 1;
    m_viewportHeight = heightPx > 0 ? heightPx : 1;
}

void Camera::setWorldHeight(float heightInWorldUnits)
{
    m_worldHeight = heightInWorldUnits;
}

void Camera::setPosition(const glm::vec2& worldCenter)
{
    m_position = worldCenter;
}

glm::mat4 Camera::viewProjectionMatrix() const
{
    const float aspect = static_cast<float>(m_viewportWidth) / static_cast<float>(m_viewportHeight);
    const float halfHeight = m_worldHeight * 0.5f;
    const float halfWidth = halfHeight * aspect;

    const float left = m_position.x - halfWidth;
    const float right = m_position.x + halfWidth;

    // World Y grows downward, so the numerically larger Y (bottom of the
    // board) must map to OpenGL's bottom of NDC (-1), and the smaller Y
    // (top of the board) to the top (+1) — the reverse of glm::ortho's
    // usual bottom/top argument order.
    const float worldTop = m_position.y - halfHeight;
    const float worldBottom = m_position.y + halfHeight;

    return glm::ortho(left, right, worldBottom, worldTop, -1.0f, 1.0f);
}
