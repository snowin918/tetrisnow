#pragma once

// Gameplay events the Face Avatar System reacts to, named after the
// brief's PLAYER_* triggers. A small, decoupled vocabulary — deliberately
// not the same type as Game/CharacterController's onAttackSuccess()-style
// methods, which drive the separate sprite-character system — so this
// subsystem stays independent of Game/ internals. GameWindow is what
// translates real match events into these (see FaceExpressionController::
// handleEvent(), the single entry point that consumes them).
enum class GameEvent
{
    PlayerAttack,   // this player landed a hit
    PlayerHit,      // this player was hit
    PlayerFrozen,   // the Freeze status effect just took hold
    PlayerUnfrozen, // ...and just ended — Frozen mirrors a continuous
                    // condition rather than a one-shot reaction, so it
                    // needs a matching "off" edge (see FaceExpressionController)
    PlayerWin,
    PlayerLose,
};
