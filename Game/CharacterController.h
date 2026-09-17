#pragma once

#include "Game/CharacterEmotion.h"

// Tracks one player's on-screen character reaction state, driven entirely
// by match events rather than by input — no rendering/timing-curve logic
// of its own (see Engine/CharacterRenderer, which reads emotion() each
// frame and decides how to draw it). The event methods are named after
// the brief's PLAYER_* triggers:
//   onAttackSuccess()  <-> PLAYER_ATTACK_SUCCESS
//   onAttackReceived() <-> PLAYER_RECEIVE_ATTACK
//   onNearDefeat()     <-> PLAYER_NEAR_DEFEAT
//   onWin()            <-> PLAYER_WIN
//   onLose()           <-> PLAYER_LOSE
//
// Happy/Angry/Surprised are transient: they show for a brief hold time,
// then fall back to Idle (or to Victory/Defeated, if the match already
// ended). Victory/Defeated persist until reset(). Frozen isn't a one-shot
// event at all — it mirrors a continuous condition (Freeze status effect
// level 4; see Game/StatusEffects.h) via setFrozen(), and outranks every
// other emotion while active.
class CharacterController
{
public:
    void update(float deltaTime);
    void reset();

    void onAttackSuccess();
    void onAttackReceived();
    void onNearDefeat();
    void onWin();
    void onLose();

    void setFrozen(bool frozen) { m_frozen = frozen; }

    CharacterEmotion emotion() const;

private:
    void triggerTransient(CharacterEmotion emotion, float holdSeconds);

    CharacterEmotion m_baseEmotion = CharacterEmotion::Idle; // Idle, Victory, or Defeated
    CharacterEmotion m_transientEmotion = CharacterEmotion::Idle;
    float m_transientHoldRemaining = 0.0f;
    bool m_frozen = false;
};
