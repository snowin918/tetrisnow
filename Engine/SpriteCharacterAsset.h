#pragma once

#include <array>
#include <string>

#include "Engine/CharacterAsset.h"
#include "Engine/OpenGLLoader.h"

class TextureManager;

// Loads one PNG per CharacterEmotion from a directory (see
// Assets/Characters/ — Idle.png, Happy.png, ...) via a TextureManager, and
// draws whichever one matches the current emotion as a textured quad.
// Falls back to a flat-colored procedural quad (Engine/CharacterRenderer's
// placeholder) for any emotion whose file failed to load, rather than
// drawing nothing.
class SpriteCharacterAsset : public CharacterAsset
{
public:
    // Loads every emotion's sprite from `directory` via `textures`.
    // `textures` must outlive this object (GameWindow owns both).
    SpriteCharacterAsset(TextureManager& textures, const std::string& directory);

    void draw(Renderer& renderer, glm::vec2 topLeft, CharacterEmotion emotion) const override;

private:
    std::array<GLuint, 7> m_textures{}; // one slot per CharacterEmotion value
};
