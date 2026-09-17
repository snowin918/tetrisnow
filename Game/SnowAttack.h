#pragma once

#include <vector>

// The three snow-attack tiers, keyed off how many lines were cleared at
// once: 1-2 lines is a quick snowball, 3 is a bigger snow bomb, and a
// Tetris (4 lines) is a powerful avalanche.
enum class SnowAttackTier
{
    Snowball,
    SnowBomb,
    Avalanche,
};

struct SnowAttack
{
    SnowAttackTier tier;
    int power; // number of garbage rows this attack adds to the target board
    int sourceLinesCleared;

    // The triggering piece's own cells, grouped by row and ordered from
    // its topmost row to its bottommost row (see
    // GameManager::lockActivePiece). GameManager::receiveAttack() uses
    // this to shape the garbage it adds so the gap echoes this piece's
    // own shape/offset instead of a plain single random column: applied
    // bottom-up, so this list's *last* entry (the piece's bottom row)
    // becomes the bottom-most new garbage row, and so on upward. Any
    // garbage rows beyond this list's length fall back to the union of
    // every column in it.
    std::vector<std::vector<int>> rowColumns;
};

// Builds the attack a given line clear produces. linesCleared is clamped
// to [1, 4] (a single lock can clear at most 4 lines).
SnowAttack createSnowAttack(int linesCleared, std::vector<std::vector<int>> rowColumns);
