#pragma once

#include <array>

#include "Game/StatusEffect.h"

// Tracks the status effects currently active against one player's board.
// Pure gameplay state — no rendering/network dependency. At most one
// instance of each StatusEffectType is active at a time; applying a type
// that's already active refreshes it (see apply()) rather than stacking a
// second instance.
//
// Freeze uses four escalating levels, deliberately designed so control is
// never yanked away except at the top level:
//   Level 1 - small slowdown     -> movementIntervalMultiplier() > 1
//   Level 2 - ice blocks         -> movementIntervalMultiplier() higher still
//   Level 3 - heavy snow storm   -> also drives visibilityImpairment() > 0
//   Level 4 - temporary freeze   -> inputLocked() true: moves, rotation,
//                                    hard drop, and gravity all pause
// ("Ice blocks" and "heavy snow storm" are the brief's flavor for levels
// 2-3; there's no separate on-board ice-block obstacle yet — that would be
// a Board-level feature to add later if wanted.)
//
// Slow behaves like a standalone "Freeze level 1" for its own duration.
// Blind drives visibilityImpairment() the same way Freeze level 3 does.
// Shield doesn't touch movement at all — it's a one-shot flag GameManager
// checks (and consumes) to negate the next incoming SnowAttack.
class StatusEffects
{
public:
    // Applies or refreshes `type`: keeps the higher of the two levels and
    // the longer of the two remaining durations, rather than stacking a
    // second instance of the same type.
    void apply(StatusEffectType type, int level, float durationSeconds);

    void update(float deltaTime);

    // Clears every active effect (e.g. on match reset).
    void clear();

    bool isActive(StatusEffectType type) const;
    int level(StatusEffectType type) const; // 0 if inactive

    // >1 means discrete actions (move/rotate) should be throttled relative
    // to normal — see GameManager's per-action cooldown, which multiplies
    // its base interval by this. Combines Slow and Freeze levels 1-3
    // (level 4 is handled by inputLocked() instead, since at that point
    // actions don't happen at all).
    float movementIntervalMultiplier() const;

    // True only at Freeze level 4: the caller should refuse moves,
    // rotation, hard drop, and gravity entirely until this clears.
    bool inputLocked() const;

    // 0 (clear) to 1 (fully obscured) — Blind or Freeze level 3. Plain
    // data; how it's actually drawn (fog overlay, darkened board, ...) is
    // entirely up to the renderer.
    float visibilityImpairment() const;

    bool shieldActive() const { return isActive(StatusEffectType::Shield); }
    // Consumes an active shield (e.g. after it blocks an attack). No-op if
    // no shield is active.
    void consumeShield();

private:
    StatusEffect* find(StatusEffectType type);
    const StatusEffect* find(StatusEffectType type) const;

    // One slot per type — fixed and small since there are only four types
    // today.
    std::array<StatusEffect, 4> m_effects{{
        StatusEffect{StatusEffectType::Freeze, 0, 0.0f},
        StatusEffect{StatusEffectType::Slow, 0, 0.0f},
        StatusEffect{StatusEffectType::Blind, 0, 0.0f},
        StatusEffect{StatusEffectType::Shield, 0, 0.0f},
    }};
};
