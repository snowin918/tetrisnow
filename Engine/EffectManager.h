#pragma once

#include <array>
#include <random>
#include <vector>

#include <glm/glm.hpp>

#include "Engine/Camera.h"
#include "Engine/ParticleSystem.h"
#include "Game/Board.h"

class Renderer;

// Owns every visual "effect" moment in the game — ambient snowfall, a
// line-clear burst, an attack's impact, and its travelling trail — plus
// the camera shake that goes with the bigger ones. GameWindow decides
// *when* these happen (called from GameManager/Match callbacks); this
// class decides *what they look like*: every particle-tuning constant
// that used to live inline in GameWindow.cpp now lives here instead, with
// no window/network/input dependency of its own.
//
// Mirrors the brief's EffectManager -> {ParticleEmitter, CameraEffect,
// AnimationEffect} shape: ParticleSystem is the emitter and Camera::
// triggerShake() is the camera effect. Piece-movement easing (Engine/
// AnimationSystem's SmoothedVec2) stays in GameWindow for now since it's
// driven by per-piece state GameWindow already owns — nothing stops it
// moving here too if a second consumer shows up.
class EffectManager
{
public:
    explicit EffectManager(Camera& camera);

    // World-space X span ambient snowflakes spawn across. Call once the
    // board layout is known — it doesn't change at runtime.
    void setAmbientSnowSpan(float minX, float maxX);

    // Advances particle physics, camera shake decay, and this frame's
    // ambient snowfall. Call once per frame in every app state (menus
    // included) so the snowy backdrop never stops.
    void update(float deltaTime);

    void draw(Renderer& renderer) const;

    // A line clear's per-block burst, colored per the cleared block
    // types, plus a camera shake scaled by how many lines cleared at once.
    void spawnBlockClearEffect(float originX, const std::vector<Board::ClearedLine>& clearedLines);

    // An attack's impact: an explosion of particles at impactX (the
    // caller's chosen landing column, not necessarily board-center) along
    // the target board's base, plus a camera shake scaled by the attack's
    // power.
    void spawnSnowExplosion(float impactX, int power);

    // A small trailing sparkle behind an in-flight attack's projectile, so
    // it reads as more than a bare moving square.
    void emitAttackTrail(glm::vec2 position, glm::vec4 color);

    // A tiny burst of ice chips at a piece's center the moment it rotates,
    // so a rotation reads as a small physical event rather than a silent
    // snap. Deliberately much smaller/shorter-lived than the clear/impact
    // bursts above.
    void emitRotationPuff(glm::vec2 position, glm::vec4 color);

    // A projectile bouncing off the target board's wall mid-flight, before
    // it's landed for real: a small ice puff plus a tiny camera "punch" —
    // deliberately lighter than spawnSnowExplosion()'s real landing so the
    // eventual impact still reads as the bigger moment. Tuned separately
    // from emitRotationPuff() even though the underlying particle shape is
    // similar, so the two can diverge later without cross-affecting feel.
    void spawnWallBounce(glm::vec2 position, glm::vec4 color);

    // Hard drop feedback: fog lingers where the piece used to be, then a
    // compact impact bang fires where it lands.
    void spawnHardDropFog(const std::array<glm::ivec2, 4>& cells, float originX, glm::vec4 color);
    void spawnHardDropImpact(const std::array<glm::ivec2, 4>& cells, float originX, glm::vec4 color);

private:
    Camera& m_camera;
    ParticleSystem m_particles;

    std::mt19937 m_ambientRng{std::random_device{}()};
    float m_ambientSnowTimer = 0.0f;
    float m_ambientMinX = 0.0f;
    float m_ambientMaxX = 0.0f;
};
