#pragma once

#include <array>
#include <string>

#include "Engine/CharacterAsset.h"
#include "Engine/OpenGLLoader.h"

class TextureManager;

// Loads one sprite sheet per CharacterEmotion from a directory (see
// Assets/Characters/ - Idle.png, Happy.png, ...). Each sheet is 8 columns x 2
// rows: columns are animation frames, rows are player 1 / player 2 characters.
// Falls back to a flat-colored procedural quad for any emotion whose file
// failed to load.
class SpriteCharacterAsset : public CharacterAsset
{
public:
    SpriteCharacterAsset(TextureManager& textures, const std::string& directory);

    void draw(
        Renderer& renderer, glm::vec2 topLeft, CharacterEmotion emotion, int playerIndex, float animationSeconds) const override;

private:
    std::array<GLuint, 9> m_textures{}; // one slot per CharacterEmotion value
};
