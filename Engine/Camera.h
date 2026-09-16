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

    glm::mat4 viewProjectionMatrix() const;

private:
    glm::vec2 m_position{0.0f, 0.0f};
    float m_worldHeight = 20.0f;
    int m_viewportWidth = 1;
    int m_viewportHeight = 1;
};
