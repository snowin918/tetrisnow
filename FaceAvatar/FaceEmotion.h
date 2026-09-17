#pragma once

// The face avatar's own emotion states — deliberately a separate type
// from Game/CharacterEmotion.h (the existing sprite-character system):
// the two are independent subsystems with different value sets (this one
// has Sad/Shocked/Defeat where the sprite system has Surprised/Attack/
// Damaged/Defeated), and Phase 4 will map gameplay events onto both
// independently rather than sharing one enum between them.
enum class FaceEmotion
{
    Neutral,
    Happy,
    Angry,
    Shocked,
    Sad,
    Frozen,
    Victory,
    Defeat,
};
