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

// Blinking (Phase 6): how often a blink starts, and how long one takes
// to close and reopen. A fixed cadence rather than a random one — no
// <random> dependency needed for a cosmetic, non-gameplay-affecting timer.
constexpr float kBlinkIntervalSeconds = 4.0f;
constexpr float kBlinkDurationSeconds = 0.14f;

// Idle breathing (Phase 6): a slow, subtle whole-mesh scale/vertical pulse.
constexpr float kBreatheCycleSeconds = 4.0f;
constexpr float kBreatheScaleAmount = 0.012f;
constexpr float kBreatheOffsetPx = 1.5f;

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
            // Also PlayerHit's reaction — a quick red "damage" flash.
            pose.entryFlashColor = glm::vec3(0.5f, 0.05f, 0.05f);
            pose.entryFlashDuration = 0.25f;
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
            // Slow icy shimmer — the brief's "snow/freeze effects" polish.
            pose.entryFlashColor = glm::vec3(0.7f, 0.9f, 1.0f);
            pose.entryFlashDuration = 1.6f;
            pose.entryFlashFrequency = 1.2f;
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
            // Pulsing gold glimmer stands in for the brief's "particles" —
            // a real particle system is a bigger addition than this
            // phase's scope, given the avatar's separate screen-space GL
            // pipeline (see FaceAvatar/FaceAvatarSystem.h) can't reuse
            // Engine/ParticleSystem's world-space one directly.
            pose.entryFlashColor = glm::vec3(1.0f, 0.9f, 0.5f);
            pose.entryFlashDuration = 1.4f;
            pose.entryFlashFrequency = 3.0f;
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
        // Unconditionally reset (not just set when >0): if the new
        // emotion has no flash of its own, this clears any flash still
        // decaying from the previous one — otherwise m_flashRemaining
        // would stay nonzero while m_target.entryFlashDuration became 0,
        // dividing by zero in apply()'s envelope calculation below.
        m_flashRemaining = m_target.entryFlashDuration > 0.0f ? m_target.entryFlashDuration : 0.0f;
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
    if (m_flashRemaining > 0.0f) {
        m_flashRemaining = std::max(0.0f, m_flashRemaining - deltaTime);
    }

    // Blinking runs continuously, independent of which emotion is active.
    if (m_blinkPhaseRemaining > 0.0f) {
        m_blinkPhaseRemaining = std::max(0.0f, m_blinkPhaseRemaining - deltaTime);
        // 0 -> 1 -> 0 over the blink's duration: closes then reopens.
        const float phase = 1.0f - m_blinkPhaseRemaining / kBlinkDurationSeconds;
        m_blinkProgress = std::sin(phase * 3.14159265f);
    } else {
        m_blinkProgress = 0.0f;
        m_nextBlinkIn -= deltaTime;
        if (m_nextBlinkIn <= 0.0f) {
            m_blinkPhaseRemaining = kBlinkDurationSeconds;
            m_nextBlinkIn = kBlinkIntervalSeconds;
        }
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

    glm::vec3 flashColor(0.0f);
    float flashStrength = 0.0f;
    if (m_flashRemaining > 0.0f && m_target.entryFlashDuration > 0.0f) {
        const float envelope = m_flashRemaining / m_target.entryFlashDuration;
        const float wave = m_target.entryFlashFrequency > 0.0f
            ? 0.5f + 0.5f * std::sin(m_animationSeconds * m_target.entryFlashFrequency * 6.2831853f)
            : 1.0f;
        flashColor = m_target.entryFlashColor;
        flashStrength = envelope * wave;
    }

    // Blinking multiplies eyeScale toward ~0 on top of whatever the
    // current pose already has it at, rather than replacing it — so a
    // wide-eyed Shocked pose still visibly blinks shut, just from a
    // wider starting point.
    const float eyeScaleWithBlink = m_eyeScale * (1.0f - m_blinkProgress * 0.92f);

    faceAvatar.setEyeScale(eyeScaleWithBlink);
    faceAvatar.setMouthOpen(m_mouthOpen);
    faceAvatar.setBrowPosition(m_browPosition);
    faceAvatar.setFaceRotation(sway + shake);
    faceAvatar.setTint(m_tint);
    faceAvatar.setContrast(m_contrast);
    faceAvatar.setFlash(flashColor, flashStrength);

    const float breatheScale = 1.0f + kBreatheScaleAmount * std::sin(m_animationSeconds * 6.2831853f / kBreatheCycleSeconds);
    const float breatheOffsetPx = kBreatheOffsetPx * std::sin(m_animationSeconds * 6.2831853f / kBreatheCycleSeconds);
    faceAvatar.setBreathing(breatheScale, breatheOffsetPx);
}
