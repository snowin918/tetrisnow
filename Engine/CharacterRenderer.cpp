#include "Engine/CharacterRenderer.h"

#include "Engine/Renderer.h"

namespace
{
glm::vec4 colorForEmotion(CharacterEmotion emotion)
{
    switch (emotion) {
        case CharacterEmotion::Idle: return {0.8f, 0.85f, 0.92f, 1.0f};
        case CharacterEmotion::Happy: return {1.0f, 0.85f, 0.2f, 1.0f};
        case CharacterEmotion::Angry: return {0.9f, 0.25f, 0.2f, 1.0f};
        case CharacterEmotion::Surprised: return {1.0f, 0.6f, 0.15f, 1.0f};
        case CharacterEmotion::Attack: return {0.45f, 0.85f, 1.0f, 1.0f};
        case CharacterEmotion::Damaged: return {0.9f, 0.45f, 0.75f, 1.0f};
        case CharacterEmotion::Frozen: return {0.6f, 0.85f, 1.0f, 1.0f};
        case CharacterEmotion::Victory: return {0.4f, 0.95f, 0.5f, 1.0f};
        case CharacterEmotion::Defeated: return {0.4f, 0.35f, 0.45f, 1.0f};
    }
    return {1.0f, 1.0f, 1.0f, 1.0f};
}
} // namespace

void drawCharacterPlaceholder(Renderer& renderer, glm::vec2 topLeft, CharacterEmotion emotion)
{
    const glm::vec4 bodyColor = colorForEmotion(emotion);
    renderer.drawQuad(topLeft, glm::vec2(kCharacterPlaceholderSize), bodyColor);

    // A couple of small, darker "eye" marks near the top so the shape
    // reads as a face rather than a bare colored square.
    constexpr float kEyeSize = 0.22f;
    const glm::vec4 eyeColor(bodyColor.r * 0.4f, bodyColor.g * 0.4f, bodyColor.b * 0.4f, 1.0f);
    const float eyeY = topLeft.y + kCharacterPlaceholderSize * 0.35f;
    renderer.drawQuad(glm::vec2(topLeft.x + kCharacterPlaceholderSize * 0.25f, eyeY), glm::vec2(kEyeSize), eyeColor);
    renderer.drawQuad(
        glm::vec2(topLeft.x + kCharacterPlaceholderSize * 0.75f - kEyeSize, eyeY), glm::vec2(kEyeSize), eyeColor);
}
