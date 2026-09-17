#pragma once

#include <glm/glm.hpp>

#include "Game/CharacterEmotion.h"

class Renderer;

// Abstracts how a character actually gets drawn, so GameWindow doesn't
// need to know whether it's looking at a 2D sprite or (someday) a 3D
// low-poly mesh — mirrors the brief's CharacterAsset -> {SpriteCharacter,
// MeshCharacter} split. Only SpriteCharacterAsset exists today (2.5D
// sprites is the chosen art direction for this project); a
// MeshCharacterAsset could be added later as a second implementation of
// this same interface without touching GameWindow or Game/
// CharacterController at all.
class CharacterAsset
{
public:
    virtual ~CharacterAsset() = default;
    virtual void draw(Renderer& renderer, glm::vec2 topLeft, CharacterEmotion emotion) const = 0;
};
