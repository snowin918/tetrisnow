#include "Game/Tetromino.h"

namespace
{
using ShapeOffsets = std::array<glm::ivec2, 4>;
using ShapeRotations = std::array<ShapeOffsets, Tetromino::kRotationStates>;

// Each shape's 4 rotation states, as (col, row) offsets within a 4x4
// bounding box. These are the standard Tetris Guideline rotation shapes,
// simplified: just the four base orientations, no SRS wall-kick data
// (GameManager applies its own simplified kick attempts instead).
const ShapeRotations& rotationsFor(BlockType type)
{
    static const ShapeRotations kI = {{
        {{{0, 1}, {1, 1}, {2, 1}, {3, 1}}},
        {{{2, 0}, {2, 1}, {2, 2}, {2, 3}}},
        {{{0, 2}, {1, 2}, {2, 2}, {3, 2}}},
        {{{1, 0}, {1, 1}, {1, 2}, {1, 3}}},
    }};
    static const ShapeRotations kO = {{
        {{{1, 0}, {2, 0}, {1, 1}, {2, 1}}},
        {{{1, 0}, {2, 0}, {1, 1}, {2, 1}}},
        {{{1, 0}, {2, 0}, {1, 1}, {2, 1}}},
        {{{1, 0}, {2, 0}, {1, 1}, {2, 1}}},
    }};
    static const ShapeRotations kT = {{
        {{{1, 0}, {0, 1}, {1, 1}, {2, 1}}},
        {{{1, 0}, {1, 1}, {2, 1}, {1, 2}}},
        {{{0, 1}, {1, 1}, {2, 1}, {1, 2}}},
        {{{1, 0}, {0, 1}, {1, 1}, {1, 2}}},
    }};
    static const ShapeRotations kS = {{
        {{{1, 0}, {2, 0}, {0, 1}, {1, 1}}},
        {{{1, 0}, {1, 1}, {2, 1}, {2, 2}}},
        {{{1, 1}, {2, 1}, {0, 2}, {1, 2}}},
        {{{0, 0}, {0, 1}, {1, 1}, {1, 2}}},
    }};
    static const ShapeRotations kZ = {{
        {{{0, 0}, {1, 0}, {1, 1}, {2, 1}}},
        {{{2, 0}, {1, 1}, {2, 1}, {1, 2}}},
        {{{0, 1}, {1, 1}, {1, 2}, {2, 2}}},
        {{{1, 0}, {0, 1}, {1, 1}, {0, 2}}},
    }};
    static const ShapeRotations kJ = {{
        {{{0, 0}, {0, 1}, {1, 1}, {2, 1}}},
        {{{1, 0}, {2, 0}, {1, 1}, {1, 2}}},
        {{{0, 1}, {1, 1}, {2, 1}, {2, 2}}},
        {{{1, 0}, {1, 1}, {0, 2}, {1, 2}}},
    }};
    static const ShapeRotations kL = {{
        {{{2, 0}, {0, 1}, {1, 1}, {2, 1}}},
        {{{1, 0}, {1, 1}, {1, 2}, {2, 2}}},
        {{{0, 1}, {1, 1}, {2, 1}, {0, 2}}},
        {{{0, 0}, {1, 0}, {1, 1}, {1, 2}}},
    }};

    switch (type) {
        case BlockType::I: return kI;
        case BlockType::O: return kO;
        case BlockType::T: return kT;
        case BlockType::S: return kS;
        case BlockType::Z: return kZ;
        case BlockType::J: return kJ;
        case BlockType::L: return kL;
        case BlockType::Empty: break;
    }
    return kO; // unreachable in practice; keeps the function total.
}
} // namespace

Tetromino::Tetromino(BlockType type, glm::ivec2 spawnPosition)
    : m_type(type)
    , m_position(spawnPosition)
{
}

void Tetromino::setRotationState(int state)
{
    m_rotationState = ((state % kRotationStates) + kRotationStates) % kRotationStates;
}

std::array<glm::ivec2, 4> Tetromino::cells() const
{
    return cellsAt(m_position, m_rotationState);
}

std::array<glm::ivec2, 4> Tetromino::cellsAt(glm::ivec2 position, int rotationState) const
{
    const int normalizedState = ((rotationState % kRotationStates) + kRotationStates) % kRotationStates;
    const ShapeOffsets& offsets = rotationsFor(m_type)[normalizedState];

    std::array<glm::ivec2, 4> result;
    for (size_t i = 0; i < 4; ++i) {
        result[i] = position + offsets[i];
    }
    return result;
}
