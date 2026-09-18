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

void CharacterController::reset()
{
    m_baseEmotion = CharacterEmotion::Idle;
    m_transientEmotion = CharacterEmotion::Idle;
    m_transientHoldRemaining = 0.0f;
    m_animationSeconds = 0.0f;
}

void CharacterController::onAttackSuccess(int clearedLines)
{
    const bool heavy = clearedLines >= 3;
    // Hold times match SpriteCharacterAsset's per-emotion flipbook duration
    // exactly, so the reaction pose is still fully visible when this falls
    // back to idle.
    triggerTransient(heavy ? CharacterEmotion::AttackStrong : CharacterEmotion::Attack,
        heavy ? 1.45f : 1.10f);
}

void CharacterController::onAttackReceived(int impactPower)
{
    const bool heavy = impactPower >= 4;
    triggerTransient(heavy ? CharacterEmotion::DamagedStrong : CharacterEmotion::Damaged,
        heavy ? 1.15f : 0.85f);
}

void CharacterController::onNearDefeat()
{
    // The new art set does not include a frozen pose, so near-defeat no longer
    // alters the rendered emotion.
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
    if (m_transientHoldRemaining > 0.0f) {
        return m_transientEmotion;
    }
    return m_baseEmotion;
}

void CharacterController::triggerTransient(CharacterEmotion emotion, float holdSeconds)
{
    if (m_baseEmotion != CharacterEmotion::Idle) return;
    // Do not interrupt an active attack or damage reaction with another transient state.
    if ((emotion == CharacterEmotion::DamagedStrong || emotion == CharacterEmotion::AttackStrong)
        && m_transientHoldRemaining > 0.0f) {
        return;
    }
    m_animationSeconds = 0.0f;
    m_transientEmotion = emotion;
    m_transientHoldRemaining = holdSeconds;
}
