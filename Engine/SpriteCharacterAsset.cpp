#include "Engine/SpriteCharacterAsset.h"

#include "Engine/CharacterRenderer.h"
#include "Engine/Renderer.h"
#include "Engine/TextureManager.h"

namespace
{
constexpr int kFramesPerEmotion = 8;
constexpr int kPlayerRows = 2;
constexpr float kFrameRate = 10.0f;

// Explicit switches rather than static_cast<size_t>(emotion) keeps the array
// indexing correct even if CharacterEmotion's declaration order ever changes.
size_t indexOf(CharacterEmotion emotion)
{
    switch (emotion) {
        case CharacterEmotion::Idle: return 0;
        case CharacterEmotion::Happy: return 1;
        case CharacterEmotion::Angry: return 2;
        case CharacterEmotion::Surprised: return 3;
        case CharacterEmotion::Attack: return 4;
        case CharacterEmotion::Damaged: return 5;
        case CharacterEmotion::Frozen: return 6;
        case CharacterEmotion::Victory: return 7;
        case CharacterEmotion::Defeated: return 8;
    }
    return 0;
}

const char* fileNameFor(CharacterEmotion emotion)
{
    switch (emotion) {
        case CharacterEmotion::Idle: return "Idle";
        case CharacterEmotion::Happy: return "Happy";
        case CharacterEmotion::Angry: return "Angry";
        case CharacterEmotion::Surprised: return "Surprised";
        case CharacterEmotion::Attack: return "Attack";
        case CharacterEmotion::Damaged: return "Damaged";
        case CharacterEmotion::Frozen: return "Frozen";
        case CharacterEmotion::Victory: return "Victory";
        case CharacterEmotion::Defeated: return "Defeated";
    }
    return "Idle";
}

constexpr CharacterEmotion kAllEmotions[] = {
    CharacterEmotion::Idle,
    CharacterEmotion::Happy,
    CharacterEmotion::Angry,
    CharacterEmotion::Surprised,
    CharacterEmotion::Attack,
    CharacterEmotion::Damaged,
    CharacterEmotion::Frozen,
    CharacterEmotion::Victory,
    CharacterEmotion::Defeated,
};
} // namespace

SpriteCharacterAsset::SpriteCharacterAsset(TextureManager& textures, const std::string& directory)
{
    for (CharacterEmotion emotion : kAllEmotions) {
        const std::string fileName = fileNameFor(emotion);
        const std::string textureName = "character_" + fileName;
        const std::string path = directory + "/" + fileName + ".png";
        m_textures[indexOf(emotion)] = textures.loadFromFile(textureName, path);
    }
}

void SpriteCharacterAsset::draw(
    Renderer& renderer, glm::vec2 topLeft, CharacterEmotion emotion, int playerIndex, float animationSeconds) const
{
    const GLuint texture = m_textures[indexOf(emotion)];
    if (texture == 0) {
        drawCharacterPlaceholder(renderer, topLeft, emotion);
        return;
    }

    const int frame = static_cast<int>(animationSeconds * kFrameRate) % kFramesPerEmotion;
    const int row = playerIndex == 1 ? 1 : 0;
    const glm::vec2 uvScale(1.0f / static_cast<float>(kFramesPerEmotion), 1.0f / static_cast<float>(kPlayerRows));
    const glm::vec2 uvOffset(static_cast<float>(frame) * uvScale.x, static_cast<float>(row) * uvScale.y);
    renderer.drawQuad(topLeft, glm::vec2(kCharacterPlaceholderSize), texture, uvOffset, uvScale);
}
