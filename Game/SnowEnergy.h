#pragma once

// A player's accumulated snow energy from line clears. Currently a
// display-only running total: Match::onLinesCleared fires a SnowAttack
// immediately on every clear rather than spending banked energy to launch
// one (a charge-and-release mechanic was considered and deliberately
// deferred as scope creep — see HANDOFF.md). Pulled out of Player into its
// own type so the concept has a clean home, independent of that decision.
class SnowEnergy
{
public:
    int total() const { return m_total; }
    void add(int amount) { m_total += amount; }
    void reset() { m_total = 0; }

private:
    int m_total = 0;
};
