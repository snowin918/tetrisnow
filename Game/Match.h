#pragma once

#include <array>
#include <random>
#include <vector>

#include "Game/Player.h"
#include "Game/SnowAttack.h"

// An in-flight snow attack, travelling from the attacker's board toward
// the target's over a short duration before it lands. Match owns these;
// the rendering layer reads them to draw a simple travelling projectile —
// Milestone 5 replaces this placeholder with real particle effects.
struct InFlightAttack
{
    SnowAttack attack;
    int targetPlayerIndex = 0;
    float elapsedSeconds = 0.0f;
    float durationSeconds = 0.4f;
};

// Coordinates a local two-player match: both players' Tetris sessions, and
// the snow attacks travelling between them. Input routing and rendering
// live outside Match — it only knows about game state and time, which
// keeps it equally usable once Milestone 6 drives one side over the
// network instead of a second local keyset.
class Match
{
public:
    Match();

    void update(float deltaTime);
    void reset();

    Player& player(int index) { return m_players[static_cast<size_t>(index)]; }
    const Player& player(int index) const { return m_players[static_cast<size_t>(index)]; }

    const std::vector<InFlightAttack>& inFlightAttacks() const { return m_inFlightAttacks; }

private:
    void onLinesCleared(int attackerIndex, int linesCleared);

    std::array<Player, 2> m_players;
    std::vector<InFlightAttack> m_inFlightAttacks;
    std::mt19937 m_rng;
};
