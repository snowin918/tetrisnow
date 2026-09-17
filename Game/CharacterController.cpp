#include "Game/CharacterController.h"

namespace
{
constexpr float kReactionHoldSeconds = 1.2f;
}

void CharacterController::update(float deltaTime)
{
    const CharacterEmotion previous = emotion();
    m_animationSeconds += deltaTime;
    if (m_transientHoldRemaining > 0.0f) {
        m_transientHoldRemaining -= deltaTime;
        if (m_transientHoldRemaining <= 0.0f) {
            m_transientHoldRemaining = 0.0f;
            m_transientEmotion = CharacterEmotion::Idle;
        }
    }
    if (emotion() != previous) m_animationSeconds = 0.0f;
}

void CharacterController::setFrozen(bool frozen)
{
    const CharacterEmotion previous = emotion();
    m_frozen = frozen;
    if (emotion() != previous) m_animationSeconds = 0.0f;
}

void CharacterController::reset()
{
    m_baseEmotion = CharacterEmotion::Idle;
    m_transientEmotion = CharacterEmotion::Idle;
    m_transientHoldRemaining = 0.0f;
    m_frozen = false;
    m_animationSeconds = 0.0f;
}

void CharacterController::onAttackSuccess()
{
    triggerTransient(CharacterEmotion::Attack, 0.72f);
}

void CharacterController::onAttackReceived()
{
    triggerTransient(CharacterEmotion::Damaged, 0.55f);
}

void CharacterController::onNearDefeat()
{
    triggerTransient(CharacterEmotion::Angry, kReactionHoldSeconds);
}

void CharacterController::onWin()
{
    m_animationSeconds = 0.0f;
    m_baseEmotion = CharacterEmotion::Victory;
    m_transientEmotion = CharacterEmotion::Idle;
    m_transientHoldRemaining = 0.0f;
}

void CharacterController::onLose()
{
    m_animationSeconds = 0.0f;
    m_baseEmotion = CharacterEmotion::Defeated;
    m_transientEmotion = CharacterEmotion::Idle;
    m_transientHoldRemaining = 0.0f;
}

CharacterEmotion CharacterController::emotion() const
{
    if (m_baseEmotion == CharacterEmotion::Victory || m_baseEmotion == CharacterEmotion::Defeated)
        return m_baseEmotion;
    if (m_frozen) {
        return CharacterEmotion::Frozen;
    }
    if (m_transientHoldRemaining > 0.0f) {
        return m_transientEmotion;
    }
    return m_baseEmotion;
}

void CharacterController::triggerTransient(CharacterEmotion emotion, float holdSeconds)
{
    if (m_baseEmotion != CharacterEmotion::Idle) return;
    // Danger warnings must not interrupt a throw or an impact reaction.
    if (emotion == CharacterEmotion::Angry && m_transientHoldRemaining > 0.0f) return;
    m_animationSeconds = 0.0f;
    m_transientEmotion = emotion;
    m_transientHoldRemaining = holdSeconds;
}
