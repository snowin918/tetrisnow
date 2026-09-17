#include "FaceAvatar/FaceExpressionController.h"

#include <algorithm>
#include <cmath>

#include "FaceAvatar/FaceAvatarSystem.h"

namespace
{
constexpr float kAttackHoldSeconds = 1.0f;
constexpr float kHitHoldSeconds = 1.0f;
constexpr float kShakeDuration = 0.35f;
constexpr float kShakeFrequency = 14.0f; // oscillations per second while shaking

// 1 - e^(-speed*dt): frame-rate-independent exponential ease, matching
// Engine/AnimationSystem::SmoothedVec2's formula (see FaceExpressionController.h
// for why it isn't reused directly).
float easeStep(float speed, float deltaTime)
{
    return 1.0f - std::exp(-speed * deltaTime);
}
} // namespace

FaceExpressionController::Pose FaceExpressionController::poseFor(FaceEmotion emotion)
{
    Pose pose;
    switch (emotion) {
        case FaceEmotion::Neutral:
            // All Pose defaults already are the neutral look.
            pose.easeSpeed = 8.0f;
            pose.idleSwayAmount = 0.02f;
            pose.idleSwaySpeed = 0.4f;
            break;

        case FaceEmotion::Happy:
            // Raised mouth corners aren't expressible with a purely
            // vertical mouthOpen, so a small positive value (a slightly
            // open smile) plus an eye squint stands in for it, per the
            // brief's "raise mouth corners, eye squint, slight head
            // movement".
            pose.eyeScale = 0.65f;
            pose.mouthOpen = 0.035f;
            pose.browPosition = 0.015f;
            pose.easeSpeed = 9.0f;
            pose.idleSwayAmount = 0.035f;
            pose.idleSwaySpeed = 0.6f;
            break;

        case FaceEmotion::Angry:
            pose.eyeScale = 0.6f; // narrow eyes
            pose.mouthOpen = -0.01f; // pressed shut
            pose.browPosition = -0.05f; // lowered eyebrows
            pose.tint = glm::vec3(1.15f, 0.95f, 0.95f); // faint red cast
            pose.contrast = 1.3f; // brief: "increase contrast"
            pose.easeSpeed = 11.0f;
            pose.idleSwayAmount = 0.015f;
            pose.idleSwaySpeed = 0.3f;
            break;

        case FaceEmotion::Shocked:
            pose.eyeScale = 1.9f; // widen eyes
            pose.mouthOpen = 0.09f; // open mouth
            pose.browPosition = 0.06f;
            pose.easeSpeed = 25.0f; // near-instant reaction
            pose.entryShakeAmount = 0.08f; // brief: "shake effect"
            break;

        case FaceEmotion::Sad:
            pose.eyeScale = 0.85f;
            pose.mouthOpen = -0.03f; // downturned
            pose.browPosition = -0.03f;
            pose.tint = glm::vec3(0.9f, 0.9f, 1.0f);
            pose.contrast = 0.9f;
            pose.easeSpeed = 5.0f; // slow droop
            pose.idleSwayAmount = 0.01f;
            pose.idleSwaySpeed = 0.2f;
            break;

        case FaceEmotion::Frozen:
            // Neutral pose — "reduced movement" is expressed by the very
            // slow ease and zero idle sway, not a distinct deformation.
            pose.tint = glm::vec3(0.6f, 0.8f, 1.0f); // brief: "blue tint"
            pose.contrast = 0.85f;
            pose.easeSpeed = 3.0f;
            pose.idleSwayAmount = 0.0f;
            pose.idleSwaySpeed = 0.0f;
            break;

        case FaceEmotion::Victory:
            pose.eyeScale = 0.6f; // big smile squint
            pose.mouthOpen = 0.05f;
            pose.browPosition = 0.02f;
            pose.tint = glm::vec3(1.15f, 1.12f, 0.85f); // brief: "glow effect"
            pose.contrast = 1.1f;
            pose.easeSpeed = 8.0f;
            pose.idleSwayAmount = 0.03f;
            pose.idleSwaySpeed = 0.5f;
            break;

        case FaceEmotion::Defeat:
            pose.eyeScale = 0.55f; // half-closed
            pose.mouthOpen = -0.05f; // frown
            pose.browPosition = -0.07f; // heavy brows
            pose.tint = glm::vec3(0.55f, 0.55f, 0.6f);
            pose.contrast = 0.8f;
            pose.easeSpeed = 4.0f; // slow, heavy
            pose.idleSwayAmount = 0.01f;
            pose.idleSwaySpeed = 0.15f;
            break;
    }
    return pose;
}

void FaceExpressionController::triggerTransient(FaceEmotion emotion, float holdSeconds)
{
    m_transientEmotion = emotion;
    m_transientHoldRemaining = holdSeconds;
}

void FaceExpressionController::handleEvent(GameEvent event)
{
    switch (event) {
        case GameEvent::PlayerAttack:
            triggerTransient(FaceEmotion::Happy, kAttackHoldSeconds);
            break;
        case GameEvent::PlayerHit:
            triggerTransient(FaceEmotion::Shocked, kHitHoldSeconds);
            break;
        case GameEvent::PlayerFrozen:
            m_frozen = true;
            break;
        case GameEvent::PlayerUnfrozen:
            m_frozen = false;
            break;
        case GameEvent::PlayerWin:
            m_baseEmotion = FaceEmotion::Victory;
            m_transientEmotion = FaceEmotion::Neutral;
            m_transientHoldRemaining = 0.0f;
            break;
        case GameEvent::PlayerLose:
            m_baseEmotion = FaceEmotion::Defeat;
            m_transientEmotion = FaceEmotion::Neutral;
            m_transientHoldRemaining = 0.0f;
            break;
    }
}

void FaceExpressionController::reset()
{
    m_baseEmotion = FaceEmotion::Neutral;
    m_transientEmotion = FaceEmotion::Neutral;
    m_transientHoldRemaining = 0.0f;
    m_frozen = false;
}

FaceEmotion FaceExpressionController::emotion() const
{
    if (m_frozen) {
        return FaceEmotion::Frozen;
    }
    if (m_transientHoldRemaining > 0.0f) {
        return m_transientEmotion;
    }
    return m_baseEmotion;
}

void FaceExpressionController::update(float deltaTime)
{
    m_animationSeconds += deltaTime;

    if (m_transientHoldRemaining > 0.0f) {
        m_transientHoldRemaining -= deltaTime;
        if (m_transientHoldRemaining <= 0.0f) {
            m_transientHoldRemaining = 0.0f;
            m_transientEmotion = FaceEmotion::Neutral;
        }
    }

    const FaceEmotion resolved = emotion();
    if (resolved != m_lastResolvedEmotion) {
        m_target = poseFor(resolved);
        if (m_target.entryShakeAmount > 0.0f) {
            m_shakeRemaining = kShakeDuration;
        }
        m_lastResolvedEmotion = resolved;
    }

    const float t = easeStep(m_target.easeSpeed, deltaTime);
    m_eyeScale += (m_target.eyeScale - m_eyeScale) * t;
    m_mouthOpen += (m_target.mouthOpen - m_mouthOpen) * t;
    m_browPosition += (m_target.browPosition - m_browPosition) * t;
    m_tint += (m_target.tint - m_tint) * t;
    m_contrast += (m_target.contrast - m_contrast) * t;

    if (m_shakeRemaining > 0.0f) {
        m_shakeRemaining = std::max(0.0f, m_shakeRemaining - deltaTime);
    }
}

void FaceExpressionController::apply(FaceAvatarSystem& faceAvatar) const
{
    const float sway = m_target.idleSwayAmount > 0.0f
        ? m_target.idleSwayAmount * std::sin(m_animationSeconds * m_target.idleSwaySpeed * 6.2831853f)
        : 0.0f;

    float shake = 0.0f;
    if (m_shakeRemaining > 0.0f) {
        const float envelope = m_shakeRemaining / kShakeDuration;
        shake = m_target.entryShakeAmount * envelope * std::sin(m_animationSeconds * kShakeFrequency * 6.2831853f);
    }

    faceAvatar.setEyeScale(m_eyeScale);
    faceAvatar.setMouthOpen(m_mouthOpen);
    faceAvatar.setBrowPosition(m_browPosition);
    faceAvatar.setFaceRotation(sway + shake);
    faceAvatar.setTint(m_tint);
    faceAvatar.setContrast(m_contrast);
}
