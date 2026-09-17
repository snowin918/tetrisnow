#pragma once

#include <string>
#include <array>
#include "Engine/CharacterAsset.h"
#include "Engine/OpenGLLoader.h"

class TextureManager;

// BattleActors.png: four columns, four rows; eight key poses per character.
// Poses are ready, windup, release, recovery, hurt, cold, victory, defeat.
class SpriteCharacterAsset : public CharacterAsset
{
public:
    SpriteCharacterAsset(TextureManager& textures, const std::string& directory);
    void update(float deltaTime) override;
    void draw(Renderer& renderer, glm::vec2 topLeft, CharacterEmotion emotion,
        int playerIndex, float animationSeconds) const override;
private:
    struct Motion {
        bool initialized = false;
        float shift = 0.0f, lift = 0.0f, stretch = 1.0f, lean = 0.0f;
        glm::vec4 tint{1.0f};
        std::array<float, 8> poses{};
    };
    mutable std::array<Motion, 2> m_motion;
    float m_time = 0.0f;
    float m_deltaTime = 0.0f;
    GLuint m_texture = 0;
};
