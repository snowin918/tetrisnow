#include "Game/SnowAttack.h"

#include <algorithm>
#include <utility>

SnowAttack createSnowAttack(int linesCleared, std::vector<std::vector<int>> rowColumns)
{
    const int clamped = std::clamp(linesCleared, 1, 4);

    SnowAttack attack;
    attack.sourceLinesCleared = clamped;
    attack.rowColumns = std::move(rowColumns);

    switch (clamped) {
        case 1: attack.tier = SnowAttackTier::Snowball; attack.power = 1; break;
        case 2: attack.tier = SnowAttackTier::Snowball; attack.power = 2; break;
        case 3: attack.tier = SnowAttackTier::SnowBomb; attack.power = 4; break;
        case 4: attack.tier = SnowAttackTier::Avalanche; attack.power = 6; break;
        default: attack.tier = SnowAttackTier::Snowball; attack.power = 1; break;
    }
    return attack;
}
