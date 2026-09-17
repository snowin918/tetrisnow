#include "Game/GameManager.h"

#include <algorithm>
#include <array>
#include <chrono>

namespace
{
constexpr glm::ivec2 kSpawnPosition{3, 0};
} // namespace

GameManager::GameManager()
    : m_activePiece(BlockType::I, kSpawnPosition) // placeholder; reset() below replaces it with a real spawn
    , m_rng(static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count()))
{
    reset();
}

void GameManager::reset()
{
    m_board.reset();
    m_score.reset();
    m_gravityAccumulator = 0.0f;
    m_gameOver = false;
    m_bag.clear();
    m_activePiece = spawnPiece();
}

void GameManager::update(float deltaTime)
{
    if (m_gameOver) {
        return;
    }

    m_gravityAccumulator += deltaTime;
    while (m_gravityAccumulator >= m_gravityIntervalSeconds) {
        m_gravityAccumulator -= m_gravityIntervalSeconds;
        if (!tryMove({0, 1})) {
            lockActivePiece();
            break; // the piece that just locked no longer exists; stop this tick's gravity loop
        }
    }
}

void GameManager::moveLeft()
{
    if (!m_gameOver) {
        tryMove({-1, 0});
    }
}

void GameManager::moveRight()
{
    if (!m_gameOver) {
        tryMove({1, 0});
    }
}

void GameManager::softDrop()
{
    if (m_gameOver) {
        return;
    }
    if (!tryMove({0, 1})) {
        lockActivePiece();
    } else {
        m_gravityAccumulator = 0.0f; // soft-dropping resets the gravity timer, as in standard Tetris
    }
}

void GameManager::hardDrop()
{
    if (m_gameOver) {
        return;
    }
    while (tryMove({0, 1})) {
        // fall until blocked
    }
    lockActivePiece();
}

void GameManager::rotateClockwise()
{
    if (!m_gameOver) {
        // Rotation state index -1 is what actually reads as clockwise on
        // screen, since board rows increase downward rather than upward.
        tryRotate(-1);
    }
}

void GameManager::rotateCounterClockwise()
{
    if (!m_gameOver) {
        tryRotate(1);
    }
}

bool GameManager::tryMove(glm::ivec2 delta)
{
    const glm::ivec2 candidatePosition = m_activePiece.position() + delta;
    const std::array<glm::ivec2, 4> candidateCells =
        m_activePiece.cellsAt(candidatePosition, m_activePiece.rotationState());
    if (!m_board.canPlaceCells(candidateCells)) {
        return false;
    }
    m_activePiece.setPosition(candidatePosition);
    return true;
}

bool GameManager::tryRotate(int direction)
{
    const int candidateState = m_activePiece.rotationState() + direction;

    // Simplified wall-kick attempts (not full SRS): try rotating in place,
    // then nudge left/right so rotation still works near walls or other
    // locked pieces.
    constexpr std::array<int, 5> kKickOffsets = {0, -1, 1, -2, 2};
    for (int offsetX : kKickOffsets) {
        const glm::ivec2 candidatePosition = m_activePiece.position() + glm::ivec2(offsetX, 0);
        const std::array<glm::ivec2, 4> candidateCells = m_activePiece.cellsAt(candidatePosition, candidateState);
        if (m_board.canPlaceCells(candidateCells)) {
            m_activePiece.setPosition(candidatePosition);
            m_activePiece.setRotationState(candidateState);
            return true;
        }
    }
    return false;
}

void GameManager::lockActivePiece()
{
    m_board.lockCells(m_activePiece.cells(), m_activePiece.type());
    const int cleared = m_board.clearFullLines();
    m_score.registerLineClear(cleared);

    m_activePiece = spawnPiece();
    if (!m_board.canPlaceCells(m_activePiece.cells())) {
        m_gameOver = true;
    }
}

Tetromino GameManager::spawnPiece()
{
    return Tetromino(drawNextType(), kSpawnPosition);
}

BlockType GameManager::drawNextType()
{
    if (m_bag.empty()) {
        refillBag();
    }
    const BlockType next = m_bag.back();
    m_bag.pop_back();
    return next;
}

void GameManager::refillBag()
{
    m_bag = {BlockType::I, BlockType::O, BlockType::T, BlockType::S, BlockType::Z, BlockType::J, BlockType::L};
    std::shuffle(m_bag.begin(), m_bag.end(), m_rng);
}
