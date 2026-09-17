#pragma once

#include <glm/glm.hpp>

// Simple 2D orthographic camera. World space uses Y increasing downward,
// matching how a Tetris board is addressed (row 0 at the top) — the camera
// takes care of flipping that into OpenGL's bottom-up NDC space so game code
// never has to think about it.
class Camera
{
public:
    void setViewportSize(int widthPx, int heightPx);

    // Number of world units visible top-to-bottom; horizontal extent follows
    // from the viewport aspect ratio.
    void setWorldHeight(float heightInWorldUnits);

    void setPosition(const glm::vec2& worldCenter);

    // Kicks off a decaying random shake (a big attack landing, a Tetris
    // clear, ...). If a stronger shake is already in progress, the weaker
    // one is ignored rather than cutting it short.
    void triggerShake(float intensity, float durationSeconds);

    // Advances the shake decay. Call once per frame.
    void update(float deltaTime);

    glm::mat4 viewProjectionMatrix() const;

private:
    glm::vec2 currentShakeOffset() const;

    glm::vec2 m_position{0.0f, 0.0f};
    float m_worldHeight = 20.0f;
    int m_viewportWidth = 1;
    int m_viewportHeight = 1;

    float m_shakeIntensity = 0.0f;
    float m_shakeDuration = 0.0f;
    float m_shakeElapsed = 0.0f;
};
