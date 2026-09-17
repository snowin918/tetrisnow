#include "Game/GameManager.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <map>

namespace
{
constexpr glm::ivec2 kSpawnPosition{3, 0};

// Groups a locked piece's own 4 cells by row, ordered top-to-bottom
// (ascending row index) — the shape SnowAttack::rowColumns needs so a
// resulting attack's garbage can echo this piece, not just a random gap.
std::vector<std::vector<int>> groupCellsByRow(const std::array<glm::ivec2, 4>& cells)
{
    std::map<int, std::vector<int>> byRow;
    for (const glm::ivec2& cell : cells) {
        byRow[cell.y].push_back(cell.x);
    }

    std::vector<std::vector<int>> rows;
    rows.reserve(byRow.size());
    for (auto& [row, columns] : byRow) {
        rows.push_back(std::move(columns));
    }
    return rows;
}
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
    const std::array<glm::ivec2, 4> lockedCells = m_activePiece.cells();
    m_board.lockCells(lockedCells, m_activePiece.type());
    const std::vector<Board::ClearedLine> clearedLines = m_board.clearFullLines();
    if (!clearedLines.empty()) {
        m_score.registerLineClear(static_cast<int>(clearedLines.size()));
        const std::vector<std::vector<int>> lockedPieceRowColumns = groupCellsByRow(lockedCells);
        for (const LinesClearedCallback& callback : m_onLinesCleared) {
            callback(clearedLines, lockedPieceRowColumns);
        }
    }

    for (const PieceLockedCallback& callback : m_onPieceLocked) {
        callback();
    }

    m_activePiece = spawnPiece();
    if (!m_board.canPlaceCells(m_activePiece.cells())) {
        triggerGameOver();
    }
}

bool GameManager::receiveAttack(const SnowAttack& attack)
{
    if (m_gameOver) {
        return false;
    }
    if (attack.power <= 0) {
        return true;
    }

    // Union of every column the triggering piece touched — used for any
    // garbage rows beyond how many rows that piece itself spanned.
    std::vector<int> footprint;
    for (const std::vector<int>& row : attack.rowColumns) {
        for (int column : row) {
            if (std::find(footprint.begin(), footprint.end(), column) == footprint.end()) {
                footprint.push_back(column);
            }
        }
    }
    if (footprint.empty()) {
        // No shape info available (shouldn't normally happen) — fall back
        // to a single random column rather than an unclearable full row.
        std::uniform_int_distribution<int> columnDist(0, Board::kWidth - 1);
        footprint.push_back(columnDist(m_rng));
    }

    // Apply the triggering piece's own rows bottom-up: its last
    // (bottom-most) row becomes the bottom-most new garbage row, and so
    // on upward, so the gaps echo that piece's shape and column offset
    // instead of a plain single random column. Any garbage rows beyond
    // the piece's own row span fall back to its full column footprint.
    std::vector<std::vector<int>> gapColumnsPerRow(static_cast<size_t>(attack.power));
    const int shapedRows = std::min(static_cast<int>(attack.rowColumns.size()), attack.power);
    for (int i = 0; i < attack.power; ++i) {
        const int fromBottom = attack.power - 1 - i; // 0 = the bottom-most new row
        if (fromBottom < shapedRows) {
            const size_t pieceRowIndex = attack.rowColumns.size() - 1 - static_cast<size_t>(fromBottom);
            gapColumnsPerRow[static_cast<size_t>(i)] = attack.rowColumns[pieceRowIndex];
        } else {
            gapColumnsPerRow[static_cast<size_t>(i)] = footprint;
        }
    }

    const bool ok = m_board.addGarbageRows(gapColumnsPerRow);
    if (!ok) {
        triggerGameOver();
        return false;
    }

    // The garbage rows just shoved the whole stack up underneath the
    // falling piece without moving it, so its old position may now be
    // invalid or simply wrong relative to the new stack. Rather than try
    // to reconcile it, drop it immediately and bring in a fresh one.
    m_activePiece = spawnPiece();
    if (!m_board.canPlaceCells(m_activePiece.cells())) {
        triggerGameOver();
        return false;
    }

    return true;
}

void GameManager::triggerGameOver()
{
    if (m_gameOver) {
        return;
    }
    m_gameOver = true;
    if (m_onGameOver) {
        m_onGameOver();
    }
}

Tetromino GameManager::spawnPiece()
{
    ++m_activePieceGeneration;
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
