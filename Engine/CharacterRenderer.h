#pragma once

#include <glm/glm.hpp>

#include "Game/CharacterEmotion.h"

class Renderer;

// The placeholder's world-space footprint, exposed so callers can center
// it (e.g. above a board of known width) without guessing its size.
constexpr float kCharacterPlaceholderSize = 4.8f;

// Minimal placeholder visualization for the 2.5D "character" layer that
// sits above each player's board (see the project brief's layered
// comic-battle diagram). Purely procedural quads, matching how the board
// itself has no sprite/texture art yet (Engine/TextureManager is unused
// by real gameplay) — this is the seam Phase 7's CharacterAsset (sprite
// vs. low-poly mesh, once real art direction is picked) plugs into, not a
// final look.
//
// A free function rather than a stateful class: there's no animation
// timeline yet (that's Phase 8's Comic Animation System) — it just reads
// Game/CharacterController's current emotion and draws a static face for
// it, at the given top-left world position.
void drawCharacterPlaceholder(Renderer& renderer, glm::vec2 topLeft, CharacterEmotion emotion);
