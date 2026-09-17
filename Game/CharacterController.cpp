#include "Game/CharacterController.h"

namespace
{
constexpr float kReactionHoldSeconds = 1.2f;
}

void CharacterController::update(float deltaTime)
{
    if (m_transientHoldRemaining > 0.0f) {
        m_transientHoldRemaining -= deltaTime;
        if (m_transientHoldRemaining <= 0.0f) {
            m_transientHoldRemaining = 0.0f;
            m_transientEmotion = CharacterEmotion::Idle;
        }
    }
}

void CharacterController::reset()
{
    m_baseEmotion = CharacterEmotion::Idle;
    m_transientEmotion = CharacterEmotion::Idle;
    m_transientHoldRemaining = 0.0f;
    m_frozen = false;
}

void CharacterController::onAttackSuccess()
{
    triggerTransient(CharacterEmotion::Happy, kReactionHoldSeconds);
}

void CharacterController::onAttackReceived()
{
    triggerTransient(CharacterEmotion::Surprised, kReactionHoldSeconds);
}

void CharacterController::onNearDefeat()
{
    triggerTransient(CharacterEmotion::Angry, kReactionHoldSeconds);
}

void CharacterController::onWin()
{
    m_baseEmotion = CharacterEmotion::Victory;
    m_transientEmotion = CharacterEmotion::Idle;
    m_transientHoldRemaining = 0.0f;
}

void CharacterController::onLose()
{
    m_baseEmotion = CharacterEmotion::Defeated;
    m_transientEmotion = CharacterEmotion::Idle;
    m_transientHoldRemaining = 0.0f;
}

CharacterEmotion CharacterController::emotion() const
{
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
    m_transientEmotion = emotion;
    m_transientHoldRemaining = holdSeconds;
}
