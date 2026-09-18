#pragma once

#include <string>
#include <array>
#include "Engine/CharacterAsset.h"
#include "Engine/OpenGLLoader.h"

class TextureManager;

// The live character art is one sprite-sheet file per reaction (idle, normal
// attack, strong attack, normal damage, strong damage, win, lose), rather
// than a single shared atlas. Each sheet is itself a 4-column x 4-row grid:
// the boy's 8-frame flipbook for that reaction fills the top two rows, the
// girl's fills the bottom two (same layout the old BattleActors.png used).
// Player 0 is drawn as the boy, player 1 as the girl, mirrored horizontally
// since both are painted facing right in the sheet (see Assets/Characters/
// README.md).
class SpriteCharacterAsset : public CharacterAsset
{
public:
    SpriteCharacterAsset(TextureManager& textures, const std::string& directory);
    void update(float deltaTime) override;
    void draw(Renderer& renderer, glm::vec2 topLeft, CharacterEmotion emotion,
        int playerIndex, float animationSeconds) const override;
private:
    float m_time = 0.0f;
    float m_deltaTime = 0.0f;
    std::array<GLuint, 7> m_textures{};
};
