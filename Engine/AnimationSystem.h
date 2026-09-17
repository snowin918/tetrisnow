#pragma once

#include <glm/glm.hpp>

// A 2D value that eases toward a target instead of snapping to it, so
// discrete, grid-stepped game state (a tetromino's board position, say)
// reads as fluid motion on screen without the underlying gameplay logic
// needing to know anything about animation.
//
// Uses frame-rate-independent exponential smoothing rather than a fixed
// lerp factor, so the same "feel" holds regardless of the frame rate.
class SmoothedVec2
{
public:
    // Teleports immediately to value, with no easing — for cases where a
    // jump genuinely is a jump (e.g. a new piece spawning) rather than a
    // continuation of movement that should animate.
    void snapTo(glm::vec2 value);

    void setTarget(glm::vec2 target) { m_target = target; }

    void update(float deltaTime);

    glm::vec2 value() const { return m_current; }

private:
    glm::vec2 m_current{0.0f, 0.0f};
    glm::vec2 m_target{0.0f, 0.0f};
    float m_speed = 18.0f; // higher = catches up to the target faster
};

// Scalar sibling of SmoothedVec2, same easing behavior — used for one-off
// "settle" style animations (e.g. a stack visually sliding into its actual
// position after a line clear or garbage insertion) that only need a single
// eased number rather than a 2D position.
class SmoothedFloat
{
public:
    void snapTo(float value);

    void setTarget(float target) { m_target = target; }

    void update(float deltaTime);

    float value() const { return m_current; }

private:
    float m_current = 0.0f;
    float m_target = 0.0f;
    float m_speed = 14.0f; // higher = catches up to the target faster
};
