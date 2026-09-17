#pragma once

#include <glm/glm.hpp>

#include "FaceAvatar/FaceEmotion.h"
#include "FaceAvatar/GameEvent.h"

class FaceAvatarSystem;

// Drives FaceAvatarSystem's deformation uniforms and shader tint/contrast
// from gameplay events (Phase 4's GameEvent — see FaceAvatar/GameEvent.h).
// Per the brief, each resolved emotion defines three things:
//   1. mesh deformation  — a target eyeScale/mouthOpen/browPosition pose
//   2. shader effects    — a tint color + contrast for the fragment shader
//   3. animation timing  — its own ease speed (how fast it settles into
//                          that pose) plus optional idle sway/shake motion
//
// Which emotion is "resolved" at any moment follows the same three-layer
// priority as Game/CharacterController::emotion() (reimplemented here for
// FaceEmotion rather than shared, since the two drive independent
// subsystems): a continuous Frozen condition outranks everything else
// while active; otherwise a brief transient reaction (PlayerAttack ->
// Happy, PlayerHit -> Shocked) holds for a moment then falls back to;
// the base state (Neutral, or Victory/Defeat once the match ends).
//
// Poses ease toward their target every frame rather than snapping, using
// frame-rate-independent exponential smoothing — the same approach as
// Engine/AnimationSystem's SmoothedVec2, reimplemented in miniature here
// since that class is vec2-only with one fixed speed, whereas every pose
// parameter here needs its own per-emotion speed. handleEvent() itself
// only updates which emotion is targeted — the face still reacts on the
// very next frame's update()/apply(), just eased rather than snapped.
//
// Phase 6 adds three "always on" polish layers, independent of which
// emotion is active: a periodic blink (multiplies eyeScale toward ~0 for
// a fraction of a second, on top of whatever the current pose's eyeScale
// already is), idle breathing (a tiny whole-mesh scale/vertical-offset
// pulse, via FaceAvatarSystem::setBreathing()), and an entry flash/glimmer
// per emotion (comic damage flash on Shocked, victory glimmer, frozen
// shimmer — via FaceAvatarSystem::setFlash(), generalizing the same
// decaying-envelope idea Shocked's shake already used).
class FaceExpressionController
{
public:
    // Gameplay entry point — GameWindow forwards match events here 1:1.
    void handleEvent(GameEvent event);

    // Returns to the neutral base state (e.g. for a rematch) — mirrors
    // Game/CharacterController::reset().
    void reset();

    void update(float deltaTime);

    // Applies the current (eased) pose and shader look to the given
    // system. Call once per frame, after update().
    void apply(FaceAvatarSystem& faceAvatar) const;

    // The resolved emotion — see the class comment for the priority order.
    FaceEmotion emotion() const;

private:
    // The complete look one FaceEmotion resolves to — see poseFor().
    struct Pose
    {
        float eyeScale = 1.0f;
        float mouthOpen = 0.0f;
        float browPosition = 0.0f;
        glm::vec3 tint{1.0f};
        float contrast = 1.0f;
        float easeSpeed = 8.0f;        // higher = settles into this pose faster
        float idleSwayAmount = 0.0f;   // radians, gentle continuous head sway
        float idleSwaySpeed = 0.0f;    // sway oscillations per second
        float entryShakeAmount = 0.0f; // radians, decaying jitter triggered on entering this emotion

        // Phase 6: an additive color pulse triggered on entering this
        // emotion. entryFlashFrequency == 0 is a single decaying flash
        // (Shocked's damage hit); > 0 makes it pulse instead, for a
        // glimmer/shimmer look (Victory, Frozen) over entryFlashDuration.
        glm::vec3 entryFlashColor{0.0f};
        float entryFlashDuration = 0.0f;
        float entryFlashFrequency = 0.0f;
    };

    static Pose poseFor(FaceEmotion emotion);
    void triggerTransient(FaceEmotion emotion, float holdSeconds);

    FaceEmotion m_baseEmotion = FaceEmotion::Neutral; // Neutral, Victory, or Defeat
    FaceEmotion m_transientEmotion = FaceEmotion::Neutral;
    float m_transientHoldRemaining = 0.0f;
    bool m_frozen = false;

    // Tracks the last emotion() result so update() can tell when it
    // changes (to retarget the pose and, for Shocked, kick off the entry
    // shake) without recomputing poseFor() every single frame.
    FaceEmotion m_lastResolvedEmotion = FaceEmotion::Neutral;
    Pose m_target;

    // Current eased values, applied to FaceAvatarSystem each frame.
    float m_eyeScale = 1.0f;
    float m_mouthOpen = 0.0f;
    float m_browPosition = 0.0f;
    glm::vec3 m_tint{1.0f};
    float m_contrast = 1.0f;

    float m_animationSeconds = 0.0f;
    float m_shakeRemaining = 0.0f;
    float m_flashRemaining = 0.0f;

    // Blinking (Phase 6): a periodic, emotion-independent eyeScale dip.
    // m_nextBlinkIn counts down to the next blink; once a blink starts,
    // m_blinkPhaseRemaining counts down through it and m_blinkProgress
    // (0 = eyes at their current pose, 1 = fully shut) is recomputed each
    // frame from where in that countdown it is.
    float m_nextBlinkIn = 2.0f;
    float m_blinkPhaseRemaining = 0.0f;
    float m_blinkProgress = 0.0f;
};
