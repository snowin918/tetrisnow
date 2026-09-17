#pragma once

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
};

// Builds the attack a given line clear produces. linesCleared is clamped
// to [1, 4] (a single lock can clear at most 4 lines).
SnowAttack createSnowAttack(int linesCleared);
