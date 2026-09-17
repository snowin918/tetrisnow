#include "Engine/SpriteCharacterAsset.h"

#include <algorithm>
#include <cmath>
#include "Engine/CharacterRenderer.h"
#include "Engine/Renderer.h"
#include "Engine/TextureManager.h"

SpriteCharacterAsset::SpriteCharacterAsset(TextureManager& textures, const std::string& directory)
    : m_texture(textures.loadFromFile("battle_actors", directory + "/BattleActors.png"))
{
}

void SpriteCharacterAsset::update(float deltaTime)
{
    m_deltaTime = std::max(0.0f, deltaTime);
    m_time += m_deltaTime;
}

void SpriteCharacterAsset::draw(Renderer& renderer, glm::vec2 topLeft,
    CharacterEmotion emotion, int playerIndex, float animationSeconds) const
{
    if (m_texture == 0) {
        drawCharacterPlaceholder(renderer, topLeft, emotion);
        return;
    }

    const float t = std::max(0.0f, animationSeconds);
    const float direction = playerIndex == 1 ? -1.0f : 1.0f;
    const float phase = m_time * 2.6f + static_cast<float>(playerIndex) * 1.3f;
    int pose = 0;
    float lift = 0.0f;
    float shift = 0.0f;
    float stretch = 1.0f;
    float lean = 0.0f;
    const auto ease = [](float value) {
        const float x = std::clamp(value, 0.0f, 1.0f);
        return x * x * (3.0f - 2.0f * x);
    };
    glm::vec4 tint(1.0f);

    switch (emotion) {
    case CharacterEmotion::Idle:
        // Slow breathing and a separate weight-shift rhythm keep the stance alive.
        stretch = 1.0f + 0.014f * std::sin(phase);
        shift = 0.045f * std::sin(phase * 0.53f);
        lean = 0.012f * std::sin(phase * 0.53f + 0.6f);
        break;
    case CharacterEmotion::Attack:
        pose = t < 0.18f ? 1 : (t < 0.38f ? 2 : 3);
        if (t < 0.18f) {
            const float windup = ease(t / 0.18f);
            shift = -0.12f * windup;
            lean = -0.035f * windup;
            stretch = 1.0f - 0.025f * windup;
        } else if (t < 0.30f) {
            const float release = ease((t - 0.18f) / 0.12f);
            shift = glm::mix(-0.12f, 0.24f, release);
            lean = glm::mix(-0.035f, 0.045f, release);
            stretch = glm::mix(0.975f, 1.015f, release);
        } else {
            const float recover = 1.0f - ease((t - 0.30f) / 0.42f);
            shift = 0.24f * recover;
            lean = 0.045f * recover;
            stretch = 1.0f + 0.015f * recover;
        }
        break;
    case CharacterEmotion::Damaged:
        pose = t < 0.36f ? 4 : 3;
        shift = -0.24f * std::sin(std::clamp(t / 0.55f, 0.0f, 1.0f) * 3.1415927f);
        lean = -0.055f * std::sin(t * 13.0f) * std::exp(-t * 5.0f);
        stretch = 1.0f - 0.035f * std::sin(std::min(t / 0.55f, 1.0f) * 3.1415927f);
        break;
    case CharacterEmotion::Surprised:
        pose = 4;
        lift = 0.13f * std::pow(std::sin(std::min(t / 0.65f, 1.0f) * 3.1415927f), 2.0f);
        stretch = 1.0f + 0.018f * std::sin(phase * 1.5f);
        break;
    case CharacterEmotion::Frozen: {
        pose = 5;
        const float shiver = 0.4f + 0.6f * std::pow(std::sin(phase * 0.6f), 2.0f);
        shift = 0.026f * std::sin(m_time * 32.0f) * shiver;
        lean = 0.009f * std::sin(m_time * 32.0f + 0.8f) * shiver;
        stretch = 0.99f + 0.005f * std::sin(phase * 1.5f);
        tint = glm::vec4(0.74f, 0.89f, 1.0f, 1.0f);
        break;
    }
    case CharacterEmotion::Angry:
        pose = 5;
        stretch = 1.0f + 0.018f * std::sin(phase * 1.8f);
        lean = 0.018f + 0.008f * std::sin(phase);
        shift = 0.025f * std::sin(phase * 0.9f);
        break;
    case CharacterEmotion::Happy:
    case CharacterEmotion::Victory: {
        pose = 6;
        // Smooth periodic hops with a rest between them (zero speed at contact).
        const float cycle = std::fmod(t, 2.4f);
        const float hop = cycle < 0.8f ? std::sin(cycle / 0.8f * 3.1415927f) : 0.0f;
        lift = 0.26f * hop * hop;
        stretch = 1.0f + 0.012f * std::sin(phase);
        lean = 0.025f * std::sin(phase * 0.7f);
        break;
    }
    case CharacterEmotion::Defeated:
        pose = 7;
        stretch = 0.98f + 0.008f * std::sin(phase * 0.65f);
        lean = -0.016f + 0.007f * std::sin(phase * 0.4f);
        break;
    }

    Motion& motion = m_motion[playerIndex == 1 ? 1 : 0];
    const float follow = motion.initialized ? 1.0f - std::exp(-m_deltaTime * 22.0f) : 1.0f;
    const float blend = motion.initialized ? 1.0f - std::exp(-m_deltaTime * 35.0f) : 1.0f;
    motion.shift = glm::mix(motion.shift, shift, follow);
    motion.lift = glm::mix(motion.lift, lift, follow);
    motion.stretch = glm::mix(motion.stretch, stretch, follow);
    motion.lean = glm::mix(motion.lean, lean, follow);
    motion.tint = glm::mix(motion.tint, tint, follow);
    for (int i = 0; i < 8; ++i)
        motion.poses[i] = glm::mix(motion.poses[i], i == pose ? 1.0f : 0.0f, blend);
    motion.initialized = true;

    const float footprint = kCharacterPlaceholderSize;
    const float shadowScale = 1.0f - motion.lift * 0.35f;
    renderer.drawSoftCircle(topLeft + glm::vec2(footprint * (0.5f - 0.30f * shadowScale), footprint - 0.13f),
        glm::vec2(footprint * 0.60f * shadowScale, 0.27f),
        glm::vec4(0.015f, 0.035f, 0.06f, 0.30f * shadowScale));
    const glm::vec2 size(footprint / std::sqrt(motion.stretch), footprint * motion.stretch);
    const float rotation = direction * motion.lean;
    // Compensate the center rotation so the lower-center pivot stays on the floor.
    const glm::vec2 pivotCorrection(std::sin(rotation) * size.y * 0.5f,
        (1.0f - std::cos(rotation)) * size.y * 0.5f);
    const glm::vec2 position = topLeft + glm::vec2((footprint - size.x) * 0.5f + direction * motion.shift,
        footprint - size.y - motion.lift) + pivotCorrection;

    for (int frame = 0; frame < 8; ++frame) {
        if (motion.poses[frame] < 0.002f) continue;
        const int cell = (playerIndex == 1 ? 8 : 0) + frame;
        constexpr float inset = 0.0008f;
        glm::vec2 uvScale(0.25f - 2.0f * inset);
        glm::vec2 uvOffset(static_cast<float>(cell % 4) * 0.25f + inset,
            static_cast<float>(cell / 4) * 0.25f + inset);
        if (playerIndex == 1) {
            uvOffset.x += uvScale.x;
            uvScale.x = -uvScale.x;
        }
        glm::vec4 color = motion.tint;
        color.a *= motion.poses[frame];
        renderer.drawQuad(position, size, m_texture, uvOffset, uvScale, color, rotation);
    }
}
