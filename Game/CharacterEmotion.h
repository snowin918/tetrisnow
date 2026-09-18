#pragma once

// The comic-reaction states a player's on-screen character can be in.
// Pure data — how each looks is entirely up to the rendering layer (see
// Engine/CharacterRenderer.h).
enum class CharacterEmotion
{
    Idle,
    Attack,
    AttackStrong,
    Damaged,
    DamagedStrong,
    Victory,
    Defeated,
};
