#pragma once

#include <glm/glm.hpp>

#include "FaceAvatar/FaceEmotion.h"

class FaceAvatarSystem;

// Drives FaceAvatarSystem's deformation uniforms and shader tint/contrast
// from a single FaceEmotion state (Phase 4's gameplay-event handlers will
// call setEmotion() — this class doesn't know about gameplay at all). Per
// the brief, each emotion defines three things:
//   1. mesh deformation  — a target eyeScale/mouthOpen/browPosition pose
//   2. shader effects    — a tint color + contrast for the fragment shader
//   3. animation timing  — its own ease speed (how fast it settles into
//                          that pose) plus optional idle sway/shake motion
//
// Poses ease toward their target every frame rather than snapping — using
// frame-rate-independent exponential smoothing, the same approach as
// Engine/AnimationSystem's SmoothedVec2, reimplemented here in miniature
// since that class is vec2-only with one fixed speed, whereas every pose
// parameter here needs its own per-emotion speed.
//
// faceRotation is driven separately from the eased pose fields: it's an
// idle sway (a slow sine wave, per-emotion amplitude/speed) plus, for
// Shocked, a short decaying shake burst on entry — motion over time
// rather than a fixed target, so easing doesn't apply to it directly.
class FaceExpressionController
{
public:
    void setEmotion(FaceEmotion emotion);
    FaceEmotion emotion() const { return m_emotion; }

    void update(float deltaTime);

    // Applies the current (eased) pose and shader look to the given
    // system. Call once per frame, after update().
    void apply(FaceAvatarSystem& faceAvatar) const;

private:
    // The complete look one FaceEmotion resolves to — see poseFor().
    struct Pose
    {
        float eyeScale = 1.0f;
        float mouthOpen = 0.0f;
        float browPosition = 0.0f;
        glm::vec3 tint{1.0f};
        float contrast = 1.0f;
        float easeSpeed = 8.0f;       // higher = settles into this pose faster
        float idleSwayAmount = 0.0f;  // radians, gentle continuous head sway
        float idleSwaySpeed = 0.0f;   // sway oscillations per second
        float entryShakeAmount = 0.0f; // radians, decaying jitter triggered on entering this emotion
    };

    static Pose poseFor(FaceEmotion emotion);

    FaceEmotion m_emotion = FaceEmotion::Neutral;
    Pose m_target;

    // Current eased values, applied to FaceAvatarSystem each frame.
    float m_eyeScale = 1.0f;
    float m_mouthOpen = 0.0f;
    float m_browPosition = 0.0f;
    glm::vec3 m_tint{1.0f};
    float m_contrast = 1.0f;

    float m_animationSeconds = 0.0f;
    float m_shakeRemaining = 0.0f;
};
