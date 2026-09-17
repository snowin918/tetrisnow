#include "Engine/AnimationSystem.h"

#include <cmath>

void SmoothedVec2::snapTo(glm::vec2 value)
{
    m_current = value;
    m_target = value;
}

void SmoothedVec2::update(float deltaTime)
{
    // 1 - e^(-speed*dt): the fraction of the remaining distance to close
    // this frame. Frame-rate independent — halving dt roughly halves the
    // step, unlike a plain "deltaTime * speed" factor which can overshoot
    // at low frame rates.
    const float t = 1.0f - std::exp(-m_speed * deltaTime);
    m_current += (m_target - m_current) * t;
}
