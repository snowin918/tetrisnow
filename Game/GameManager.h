#pragma once

#include <functional>
#include <random>
#include <vector>

#include "Game/Board.h"
#include "Game/ScoreSystem.h"
#include "Game/SnowAttack.h"
#include "Game/Tetromino.h"

// Orchestrates one player's Tetris session: owns the Board, the currently
// falling Tetromino, gravity timing, and scoring. Exposes only
// intent-level actions (moveLeft, rotateClockwise, ...) — Board and
// Tetromino never know about input or timing themselves.
class GameManager
{
public:
    GameManager();

    // Advances gravity; locks the active piece and spawns the next one if
    // it can no longer fall. No-op once the game is over.
    void update(float deltaTime);

    void moveLeft();
    void moveRight();
    void softDrop();
    void hardDrop();
    void rotateClockwise();
    void rotateCounterClockwise();

    void reset();

    // Applies an incoming attack's garbage rows to this board. Returns
    // false if it caused a board overflow (this triggers game over).
    bool receiveAttack(const SnowAttack& attack);

    const Board& board() const { return m_board; }
    const Tetromino& activePiece() const { return m_activePiece; }
    const ScoreSystem& score() const { return m_score; }
    bool isGameOver() const { return m_gameOver; }

    // Increments every time a new active piece spawns (including on
    // reset). Lets rendering tell "the piece moved" apart from "a new
    // piece appeared" — e.g. to snap animation state instead of sliding it
    // in from the previous piece's position.
    int activePieceGeneration() const { return m_activePieceGeneration; }

    // Fired synchronously, with the cleared rows' original position/
    // contents, whenever a lock clears at least one line. Multiple
    // subscribers (Match reacts with a SnowAttack; the rendering layer
    // spawns clear-effect particles) without GameManager knowing about
    // either.
    using LinesClearedCallback = std::function<void(const std::vector<Board::ClearedLine>&)>;
    void addOnLinesCleared(LinesClearedCallback callback) { m_onLinesCleared.push_back(std::move(callback)); }

    // Fired once per lock, right after any cleared lines are removed —
    // i.e. whenever this board's settled grid actually changed, clear or
    // not. Milestone 6's host uses this to know when to push a fresh
    // board snapshot to the network client.
    using PieceLockedCallback = std::function<void()>;
    void addOnPieceLocked(PieceLockedCallback callback) { m_onPieceLocked.push_back(std::move(callback)); }

    // Fired once, the moment the game transitions into game-over.
    using GameOverCallback = std::function<void()>;
    void setOnGameOver(GameOverCallback callback) { m_onGameOver = std::move(callback); }

private:
    bool tryMove(glm::ivec2 delta);
    bool tryRotate(int direction);
    void lockActivePiece();
    Tetromino spawnPiece();
    BlockType drawNextType();
    void refillBag();
    void triggerGameOver();

    Board m_board;
    Tetromino m_activePiece;
    ScoreSystem m_score;

    float m_gravityAccumulator = 0.0f;
    float m_gravityIntervalSeconds = 0.8f;
    bool m_gameOver = false;

    std::vector<BlockType> m_bag; // 7-bag randomizer: shuffled, drawn from the back
    std::mt19937 m_rng;
    int m_activePieceGeneration = 0;

    std::vector<LinesClearedCallback> m_onLinesCleared;
    std::vector<PieceLockedCallback> m_onPieceLocked;
    GameOverCallback m_onGameOver;
};
