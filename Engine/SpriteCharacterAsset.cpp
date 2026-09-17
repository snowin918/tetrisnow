#include "Engine/SpriteCharacterAsset.h"

#include "Engine/CharacterRenderer.h"
#include "Engine/Renderer.h"
#include "Engine/TextureManager.h"

namespace
{
// Explicit switches rather than static_cast<size_t>(emotion) — keeps the
// array indexing correct even if CharacterEmotion's declaration order
// ever changes.
size_t indexOf(CharacterEmotion emotion)
{
    switch (emotion) {
        case CharacterEmotion::Idle: return 0;
        case CharacterEmotion::Happy: return 1;
        case CharacterEmotion::Angry: return 2;
        case CharacterEmotion::Surprised: return 3;
        case CharacterEmotion::Frozen: return 4;
        case CharacterEmotion::Victory: return 5;
        case CharacterEmotion::Defeated: return 6;
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
        case CharacterEmotion::Frozen: return "Frozen";
        case CharacterEmotion::Victory: return "Victory";
        case CharacterEmotion::Defeated: return "Defeated";
    }
    return "Idle";
}

constexpr CharacterEmotion kAllEmotions[] = {
    CharacterEmotion::Idle,      CharacterEmotion::Happy,    CharacterEmotion::Angry, CharacterEmotion::Surprised,
    CharacterEmotion::Frozen, CharacterEmotion::Victory, CharacterEmotion::Defeated,
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

void SpriteCharacterAsset::draw(Renderer& renderer, glm::vec2 topLeft, CharacterEmotion emotion) const
{
    const GLuint texture = m_textures[indexOf(emotion)];
    if (texture == 0) {
        drawCharacterPlaceholder(renderer, topLeft, emotion); // load failed - fall back rather than draw nothing
        return;
    }
    renderer.drawQuad(topLeft, glm::vec2(kCharacterPlaceholderSize), texture);
}
