#pragma once

#include <array>

#include <glm/glm.hpp>

#include "Game/BlockType.h"

// A falling tetromino: a shape (I/O/T/S/Z/J/L), one of 4 rotation states,
// and a position in board space (the top-left of its 4x4 bounding box).
//
// Tetromino only knows its own geometry — it has no idea whether a given
// position/rotation is legal on a particular Board. GameManager computes
// candidate cells via cellsAt() and asks Board::canPlaceCells() before
// committing a move via setPosition()/setRotationState().
class Tetromino
{
public:
    static constexpr int kRotationStates = 4;

    Tetromino(BlockType type, glm::ivec2 spawnPosition);

    BlockType type() const { return m_type; }
    glm::ivec2 position() const { return m_position; }
    int rotationState() const { return m_rotationState; }

    void setPosition(glm::ivec2 position) { m_position = position; }
    void setRotationState(int state);

    // Absolute board-space cells at the current position/rotation.
    std::array<glm::ivec2, 4> cells() const;

    // Absolute board-space cells at a hypothetical position/rotation —
    // used to test moves/rotations before committing them.
    std::array<glm::ivec2, 4> cellsAt(glm::ivec2 position, int rotationState) const;

private:
    BlockType m_type;
    glm::ivec2 m_position;
    int m_rotationState = 0;
};
