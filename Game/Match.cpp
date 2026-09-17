#include "Game/Match.h"

#include <chrono>
#include <cstdio>

namespace
{
constexpr int kSnowEnergyPerLine = 10;
} // namespace

Match::Match()
    : m_players{Player("Player 1"), Player("Player 2")}
    , m_rng(static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count()))
{
    for (int i = 0; i < 2; ++i) {
        m_players[static_cast<size_t>(i)].gameManager().addOnLinesCleared(
            [this, i](const std::vector<Board::ClearedLine>& clearedLines, const std::vector<std::vector<int>>& rowColumns) {
                onLinesCleared(i, static_cast<int>(clearedLines.size()), rowColumns);
            });

        m_players[static_cast<size_t>(i)].gameManager().setOnGameOver([this, i] {
            const int winnerIndex = 1 - i;
            std::fprintf(
                stderr, "%s's board overflowed — %s wins!\n", m_players[static_cast<size_t>(i)].name().c_str(),
                m_players[static_cast<size_t>(winnerIndex)].name().c_str());
        });
    }
}

void Match::update(float deltaTime)
{
    for (Player& p : m_players) {
        if (!p.gameManager().isGameOver()) {
            p.gameManager().update(deltaTime);
        }
    }

    for (auto it = m_inFlightAttacks.begin(); it != m_inFlightAttacks.end();) {
        it->elapsedSeconds += deltaTime;
        if (it->elapsedSeconds >= it->durationSeconds) {
            m_players[static_cast<size_t>(it->targetPlayerIndex)].receiveAttack(it->attack);
            if (m_onAttackLanded) {
                m_onAttackLanded(it->targetPlayerIndex, it->attack);
            }
            it = m_inFlightAttacks.erase(it);
        } else {
            ++it;
        }
    }
}

void Match::reset()
{
    for (Player& p : m_players) {
        p.reset();
    }
    m_inFlightAttacks.clear();
}

void Match::onLinesCleared(int attackerIndex, int linesCleared, const std::vector<std::vector<int>>& rowColumns)
{
    if (linesCleared <= 0) {
        return;
    }

    m_players[static_cast<size_t>(attackerIndex)].addSnowEnergy(linesCleared * kSnowEnergyPerLine);

    InFlightAttack inFlight;
    inFlight.attack = createSnowAttack(linesCleared, rowColumns);
    inFlight.targetPlayerIndex = 1 - attackerIndex;
    m_inFlightAttacks.push_back(inFlight);

    std::fprintf(
        stderr, "%s cleared %d line(s) -> attack power %d heading to %s\n",
        m_players[static_cast<size_t>(attackerIndex)].name().c_str(), linesCleared, inFlight.attack.power,
        m_players[static_cast<size_t>(inFlight.targetPlayerIndex)].name().c_str());
}
