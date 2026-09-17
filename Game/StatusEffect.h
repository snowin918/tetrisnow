#pragma once

// The kinds of status effect a player's board can be under. Distinct from
// SnowAttackType (Game/SnowAttack.h): an attack is the projectile that
// crosses the board, while a status effect is a lingering condition
// anything (an attack today, a character ability later) can apply to a
// board afterward.
enum class StatusEffectType
{
    Freeze,
    Slow,
    Blind,
    Shield,
};

// One active instance of a status effect. `level` is a 1-based intensity
// specific to the type — only Freeze currently distinguishes levels 1-4
// (see StatusEffects::apply in Game/StatusEffects.h); the other types
// treat any level as simply "on".
struct StatusEffect
{
    StatusEffectType type;
    int level = 0; // 0 means inactive
    float remainingSeconds = 0.0f;
};
