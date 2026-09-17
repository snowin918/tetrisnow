#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

#include "Game/Board.h"
#include "Game/BlockType.h"
#include "Game/SnowAttack.h"

// Wire format for Milestone 6's host-authoritative LAN link. The host runs
// the real Match/GameManager simulation for both players unchanged; the
// client only sends its own input and receives small, semantic messages
// describing what changed. A player's board grid only crosses the wire
// when it actually changes (a lock, a clear, an incoming attack, or a
// reset) — never as a full snapshot every frame. The one message sent
// every host tick (LiveState) carries just the two falling pieces and any
// in-flight attacks, not board content.
namespace Protocol
{

enum class MessageType : uint8_t
{
    BoardSnapshot = 1,  // host -> client: one player's full settled grid
    LiveState = 2,      // host -> client: both falling pieces + in-flight attacks
    LinesClearedFx = 3, // host -> client: one lock's cleared rows, for particles
    AttackLandedFx = 4, // host -> client: an attack resolving, for particles/shake
    MatchReset = 5,     // host -> client: the match was reset
    InputState = 6,     // client -> host: currently-held movement keys
    InputAction = 7,    // client -> host: a one-shot discrete action
};

enum class InputActionType : uint8_t
{
    RotateCW = 0,
    HardDrop = 1,
    ResetRequest = 2,
};

struct BoardSnapshotMsg
{
    int playerIndex = 0;
    std::array<std::array<BlockType, Board::kWidth>, Board::kHeight> cells{};
};

struct PieceStateMsg
{
    bool gameOver = false;
    BlockType type = BlockType::I;
    glm::ivec2 position{0, 0};
    int rotationState = 0;
    int generation = -1;
    BlockType nextType = BlockType::Empty; // for the client's next-piece HUD preview
    int score = 0;                         // HUD display only
    int snowEnergy = 0;                    // HUD display only
};

// Mirrors Game/Match.h's InFlightAttack shape (minus which player is the
// source — with exactly two players that's always "the other one").
struct InFlightAttackMsg
{
    SnowAttackType type = SnowAttackType::Snowball;
    int power = 0;
    int sourceLinesCleared = 0;
    int targetPlayerIndex = 0;
    float elapsedSeconds = 0.0f;
    float durationSeconds = 0.0f;
};

struct LiveStateMsg
{
    std::array<PieceStateMsg, 2> players;
    std::vector<InFlightAttackMsg> inFlightAttacks;
};

struct LinesClearedFxMsg
{
    int playerIndex = 0;
    std::vector<Board::ClearedLine> clearedLines;
};

struct AttackLandedFxMsg
{
    int targetPlayerIndex = 0;
    SnowAttack attack{};
};

struct InputStateMsg
{
    bool left = false;
    bool right = false;
    bool down = false;
};

struct InputActionMsg
{
    InputActionType action = InputActionType::RotateCW;
};

std::vector<uint8_t> encode(const BoardSnapshotMsg& msg);
std::vector<uint8_t> encode(const LiveStateMsg& msg);
std::vector<uint8_t> encode(const LinesClearedFxMsg& msg);
std::vector<uint8_t> encode(const AttackLandedFxMsg& msg);
std::vector<uint8_t> encodeMatchReset();
std::vector<uint8_t> encode(const InputStateMsg& msg);
std::vector<uint8_t> encode(const InputActionMsg& msg);

// Reads the leading MessageType byte so the caller knows which decode*
// function to call. `bytes` must be non-empty (every encoded message is
// at least one byte).
MessageType peekType(const std::vector<uint8_t>& bytes);

BoardSnapshotMsg decodeBoardSnapshot(const std::vector<uint8_t>& bytes);
LiveStateMsg decodeLiveState(const std::vector<uint8_t>& bytes);
LinesClearedFxMsg decodeLinesClearedFx(const std::vector<uint8_t>& bytes);
AttackLandedFxMsg decodeAttackLandedFx(const std::vector<uint8_t>& bytes);
InputStateMsg decodeInputState(const std::vector<uint8_t>& bytes);
InputActionMsg decodeInputAction(const std::vector<uint8_t>& bytes);

} // namespace Protocol
