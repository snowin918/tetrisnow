#include "Engine/Camera.h"

#include <cmath>

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

void Camera::triggerShake(float intensity, float durationSeconds)
{
    // Take the stronger shake, so a big hit isn't cut short by a smaller
    // one arriving moments later.
    if (intensity >= m_shakeIntensity) {
        m_shakeIntensity = intensity;
        m_shakeDuration = durationSeconds;
        m_shakeElapsed = 0.0f;
    }
}

void Camera::update(float deltaTime)
{
    if (m_shakeElapsed < m_shakeDuration) {
        m_shakeElapsed += deltaTime;
    }
}

glm::vec2 Camera::currentShakeOffset() const
{
    if (m_shakeDuration <= 0.0f || m_shakeElapsed >= m_shakeDuration) {
        return glm::vec2(0.0f);
    }

    const float remaining01 = 1.0f - (m_shakeElapsed / m_shakeDuration); // 1 -> 0 decay
    const float magnitude = m_shakeIntensity * remaining01;

    // Two different-frequency sines give a quick wiggle that doesn't read
    // as an obviously repeating pattern over a shake's short lifetime.
    const float x = std::sin(m_shakeElapsed * 55.0f) * magnitude;
    const float y = std::cos(m_shakeElapsed * 47.0f) * magnitude;
    return glm::vec2(x, y);
}

float Camera::worldToScreenX(float worldX, float screenWidthPx) const
{
    const float aspect = static_cast<float>(m_viewportWidth) / static_cast<float>(m_viewportHeight);
    const float halfWidth = (m_worldHeight * 0.5f) * aspect;
    const float left = m_position.x - halfWidth;
    const float right = m_position.x + halfWidth;
    return (worldX - left) / (right - left) * screenWidthPx;
}

glm::mat4 Camera::viewProjectionMatrix() const
{
    const float aspect = static_cast<float>(m_viewportWidth) / static_cast<float>(m_viewportHeight);
    const float halfHeight = m_worldHeight * 0.5f;
    const float halfWidth = halfHeight * aspect;

    const glm::vec2 shakenPosition = m_position + currentShakeOffset();

    const float left = shakenPosition.x - halfWidth;
    const float right = shakenPosition.x + halfWidth;

    // World Y grows downward, so the numerically larger Y (bottom of the
    // board) must map to OpenGL's bottom of NDC (-1), and the smaller Y
    // (top of the board) to the top (+1) — the reverse of glm::ortho's
    // usual bottom/top argument order.
    const float worldTop = shakenPosition.y - halfHeight;
    const float worldBottom = shakenPosition.y + halfHeight;

    return glm::ortho(left, right, worldBottom, worldTop, -1.0f, 1.0f);
}
