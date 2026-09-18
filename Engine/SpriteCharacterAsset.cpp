#include "Engine/SpriteCharacterAsset.h"

#include <algorithm>
#include <cmath>
#include "Engine/CharacterRenderer.h"
#include "Engine/Renderer.h"
#include "Engine/TextureManager.h"

SpriteCharacterAsset::SpriteCharacterAsset(TextureManager& textures, const std::string& directory)
    :m_textures{ {
        textures.loadFromFile("idle", directory + "/idle.png"),
        textures.loadFromFile("attack", directory + "/attack1.png"),
        textures.loadFromFile("attack_strong", directory + "/attack2.png"),
        textures.loadFromFile("damaged", directory + "/damage1.png"),
        textures.loadFromFile("damaged_strong", directory + "/damage2.png"),
        textures.loadFromFile("victory", directory + "/win.png"),
        textures.loadFromFile("defeated", directory + "/lose.png")
    } }
{
}

void SpriteCharacterAsset::update(float deltaTime)
{
    m_deltaTime = std::max(0.0f, deltaTime);
    m_time += m_deltaTime;
}

namespace
{
// Each reaction sheet packs its 8-frame flipbook across 4 columns.
// cycles == 0 loops the 8 frames forever (Idle); cycles == N plays through
// N full 8-frame passes and then holds on the last frame. Attack/Damaged
// durations match CharacterController's transient hold times exactly, so
// the flipbook lands on its final frame just as the controller falls back
// to idle.
struct FrameTiming {
    float frameDuration;
    int cycles;
};

FrameTiming frameTimingFor(CharacterEmotion emotion)
{
    switch (emotion) {
        case CharacterEmotion::Idle: return {0.20f, 0};
        case CharacterEmotion::Attack: return {1.10f / 8.0f, 1};
        case CharacterEmotion::AttackStrong: return {1.45f / 8.0f, 1};
        case CharacterEmotion::Damaged: return {0.85f / 8.0f, 1};
        case CharacterEmotion::DamagedStrong: return {1.15f / 8.0f, 1};
        // Play the victory/defeat flipbook twice, then freeze on its last
        // frame, rather than looping forever -- these are end-of-match
        // poses, not idle chatter.
        case CharacterEmotion::Victory: return {0.15f, 2};
        case CharacterEmotion::Defeated: return {0.22f, 2};
    }
    return {0.20f, 0};
}
} // namespace

void SpriteCharacterAsset::draw(Renderer& renderer, glm::vec2 topLeft,
    CharacterEmotion emotion, int playerIndex, float animationSeconds) const
{
    const size_t index = static_cast<size_t>(emotion);
    if (index >= m_textures.size()) {
        return;
    }

    const GLuint texture = m_textures[index];
    if (texture == 0) {
        // Sheet failed to load (or hasn't been generated yet) — fall back to
        // the procedural placeholder rather than drawing nothing.
        drawCharacterPlaceholder(renderer, topLeft, emotion);
        return;
    }

    const FrameTiming timing = frameTimingFor(emotion);
    const float t = std::max(0.0f, animationSeconds);
    const float cycleSeconds = timing.frameDuration * 8.0f;
    float framePos;
    if (timing.cycles <= 0) {
        framePos = std::fmod(t, cycleSeconds);
    } else {
        const float totalSeconds = cycleSeconds * static_cast<float>(timing.cycles);
        framePos = t < totalSeconds ? std::fmod(t, cycleSeconds) : cycleSeconds - 0.0001f;
    }
    const int frame = std::clamp(static_cast<int>(framePos / timing.frameDuration), 0, 7);

    // Boy fills cells 0-7 (the sheet's top two rows), girl fills cells 8-15
    // (the bottom two); both are painted facing right, so player 1 (the
    // girl) is mirrored horizontally to face the boy across the arena.
    const bool isGirl = playerIndex == 1;
    const int cell = (isGirl ? 8 : 0) + frame;

    constexpr float insetX = 0.00f;
    constexpr float insetY = 0.00f;
    glm::vec2 uvScale(0.25f - 2.0f * insetX, 0.25f - 2.0f * insetY);
    glm::vec2 uvOffset(
        static_cast<float>(cell % 4) * 0.25f + insetX, static_cast<float>(cell / 4) * 0.25f + insetY);
    if (isGirl) {
        uvOffset.x += uvScale.x;
        uvScale.x = -uvScale.x;
    }

    const float footprint = kCharacterPlaceholderSize * kCharacterRenderScale;
    const glm::vec2 size(footprint, footprint * 1.35f);
    renderer.drawQuad(topLeft, size, texture, uvOffset, uvScale, glm::vec4(1.0f), 0.0f);
}
