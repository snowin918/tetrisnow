#pragma once

#include <cstdint>

// Identifies a locked board cell or a falling piece's shape. Empty marks an
// unoccupied board cell; it is never a valid Tetromino type.
enum class BlockType : uint8_t
{
    Empty,
    I,
    O,
    T,
    S,
    Z,
    J,
    L,
};
