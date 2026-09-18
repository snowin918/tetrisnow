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
// The runtime states now follow the new character-art set: idle, normal
// attack, heavy attack, damaged, heavy damaged, victory, and defeat.
// Old generic reactions like Happy/Angry/Surprised are intentionally not used.
class CharacterController
{
public:
    void update(float deltaTime);
    void reset();

    void onAttackSuccess(int clearedLines = 0);
    void onAttackReceived(int impactPower = 0);
    void onNearDefeat();
    void onWin();
    void onLose();

    float animationSeconds() const { return m_animationSeconds; }

    CharacterEmotion emotion() const;

private:
    void triggerTransient(CharacterEmotion emotion, float holdSeconds);

    CharacterEmotion m_baseEmotion = CharacterEmotion::Idle; // Idle, Victory, or Defeated
    CharacterEmotion m_transientEmotion = CharacterEmotion::Idle;
    float m_transientHoldRemaining = 0.0f;
    float m_animationSeconds = 0.0f;
};
