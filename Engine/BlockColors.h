#pragma once

#include <glm/glm.hpp>

#include "Game/BlockType.h"

// Maps a piece/block type to its render color. Deliberately kept out of
// Game/ — BlockType itself carries no color, keeping gameplay independent
// of rendering (Milestone 3 requirement). Shared by GameWindow's board
// rendering and the UI/Hud next-piece preview so both agree on color.
inline glm::vec4 colorForBlockType(BlockType type)
{
    switch (type) {
        case BlockType::I: return {0.2f, 0.85f, 0.9f, 1.0f};
        case BlockType::O: return {0.95f, 0.9f, 0.2f, 1.0f};
        case BlockType::T: return {0.65f, 0.25f, 0.85f, 1.0f};
        case BlockType::S: return {0.3f, 0.85f, 0.3f, 1.0f};
        case BlockType::Z: return {0.9f, 0.25f, 0.25f, 1.0f};
        case BlockType::J: return {0.25f, 0.35f, 0.95f, 1.0f};
        case BlockType::L: return {0.95f, 0.6f, 0.1f, 1.0f};
        case BlockType::Snow: return {0.75f, 0.82f, 0.9f, 1.0f};
        case BlockType::Empty: break;
    }
    return {1.0f, 1.0f, 1.0f, 1.0f};
}
