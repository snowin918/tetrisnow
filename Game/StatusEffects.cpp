#include "Game/StatusEffects.h"

#include <algorithm>

void StatusEffects::apply(StatusEffectType type, int level, float durationSeconds)
{
    StatusEffect* effect = find(type);
    if (!effect) {
        return;
    }
    effect->level = std::max(effect->level, level);
    effect->remainingSeconds = std::max(effect->remainingSeconds, durationSeconds);
}

void StatusEffects::update(float deltaTime)
{
    for (StatusEffect& effect : m_effects) {
        if (effect.level <= 0) {
            continue;
        }
        effect.remainingSeconds -= deltaTime;
        if (effect.remainingSeconds <= 0.0f) {
            effect.level = 0;
            effect.remainingSeconds = 0.0f;
        }
    }
}

void StatusEffects::clear()
{
    for (StatusEffect& effect : m_effects) {
        effect.level = 0;
        effect.remainingSeconds = 0.0f;
    }
}

bool StatusEffects::isActive(StatusEffectType type) const
{
    const StatusEffect* effect = find(type);
    return effect && effect->level > 0;
}

int StatusEffects::level(StatusEffectType type) const
{
    const StatusEffect* effect = find(type);
    return effect ? effect->level : 0;
}

float StatusEffects::movementIntervalMultiplier() const
{
    float multiplier = 1.0f;
    if (isActive(StatusEffectType::Slow)) {
        multiplier = std::max(multiplier, 1.5f);
    }

    switch (level(StatusEffectType::Freeze)) {
        case 1: multiplier = std::max(multiplier, 1.5f); break;
        case 2: multiplier = std::max(multiplier, 2.0f); break;
        case 3: multiplier = std::max(multiplier, 2.5f); break;
        default: break; // 0 (inactive) or 4 (handled by inputLocked instead)
    }
    return multiplier;
}

bool StatusEffects::inputLocked() const
{
    return level(StatusEffectType::Freeze) >= 4;
}

float StatusEffects::visibilityImpairment() const
{
    float impairment = 0.0f;
    if (isActive(StatusEffectType::Blind)) {
        impairment = std::max(impairment, 0.6f);
    }
    if (level(StatusEffectType::Freeze) >= 3) {
        impairment = std::max(impairment, 0.8f);
    }
    return std::clamp(impairment, 0.0f, 1.0f);
}

void StatusEffects::consumeShield()
{
    if (StatusEffect* effect = find(StatusEffectType::Shield)) {
        effect->level = 0;
        effect->remainingSeconds = 0.0f;
    }
}

StatusEffect* StatusEffects::find(StatusEffectType type)
{
    for (StatusEffect& effect : m_effects) {
        if (effect.type == type) {
            return &effect;
        }
    }
    return nullptr;
}

const StatusEffect* StatusEffects::find(StatusEffectType type) const
{
    for (const StatusEffect& effect : m_effects) {
        if (effect.type == type) {
            return &effect;
        }
    }
    return nullptr;
}
