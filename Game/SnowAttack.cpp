#include "Game/SnowAttack.h"

#include <algorithm>

SnowAttack createSnowAttack(int linesCleared)
{
    const int clamped = std::clamp(linesCleared, 1, 4);

    switch (clamped) {
        case 1: return SnowAttack{SnowAttackTier::Snowball, 1, clamped};
        case 2: return SnowAttack{SnowAttackTier::Snowball, 2, clamped};
        case 3: return SnowAttack{SnowAttackTier::SnowBomb, 4, clamped};
        case 4: return SnowAttack{SnowAttackTier::Avalanche, 6, clamped};
        default: return SnowAttack{SnowAttackTier::Snowball, 1, clamped};
    }
}
