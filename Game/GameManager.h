#pragma once

#include <random>
#include <vector>

#include "Game/Board.h"
#include "Game/ScoreSystem.h"
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

    const Board& board() const { return m_board; }
    const Tetromino& activePiece() const { return m_activePiece; }
    const ScoreSystem& score() const { return m_score; }
    bool isGameOver() const { return m_gameOver; }

private:
    bool tryMove(glm::ivec2 delta);
    bool tryRotate(int direction);
    void lockActivePiece();
    Tetromino spawnPiece();
    BlockType drawNextType();
    void refillBag();

    Board m_board;
    Tetromino m_activePiece;
    ScoreSystem m_score;

    float m_gravityAccumulator = 0.0f;
    float m_gravityIntervalSeconds = 0.8f;
    bool m_gameOver = false;

    std::vector<BlockType> m_bag; // 7-bag randomizer: shuffled, drawn from the back
    std::mt19937 m_rng;
};
