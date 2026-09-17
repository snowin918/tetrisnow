#include "Game/Board.h"

Board::Board()
{
    reset();
}

bool Board::canPlaceCell(int col, int row) const
{
    if (col < 0 || col >= kWidth) {
        return false;
    }
    if (row >= kHeight) {
        return false;
    }
    if (row < 0) {
        return true; // hidden spawn buffer above the board
    }
    return m_cells[row][col] == BlockType::Empty;
}

bool Board::canPlaceCells(const std::array<glm::ivec2, 4>& cells) const
{
    for (const glm::ivec2& cell : cells) {
        if (!canPlaceCell(cell.x, cell.y)) {
            return false;
        }
    }
    return true;
}

BlockType Board::cellAt(int col, int row) const
{
    if (col < 0 || col >= kWidth || row < 0 || row >= kHeight) {
        return BlockType::Empty;
    }
    return m_cells[row][col];
}

void Board::lockCells(const std::array<glm::ivec2, 4>& cells, BlockType type)
{
    for (const glm::ivec2& cell : cells) {
        if (cell.x >= 0 && cell.x < kWidth && cell.y >= 0 && cell.y < kHeight) {
            m_cells[cell.y][cell.x] = type;
        }
    }
}

int Board::clearFullLines()
{
    int clearedCount = 0;

    for (int row = kHeight - 1; row >= 0; --row) {
        if (!isRowFull(row)) {
            continue;
        }

        // Shift every row above this one down by one, then clear the top row.
        for (int shiftRow = row; shiftRow > 0; --shiftRow) {
            m_cells[shiftRow] = m_cells[shiftRow - 1];
        }
        m_cells[0].fill(BlockType::Empty);

        ++clearedCount;
        ++row; // re-check this row index, which now holds the row that was above it
    }

    return clearedCount;
}

void Board::reset()
{
    for (auto& row : m_cells) {
        row.fill(BlockType::Empty);
    }
}

bool Board::isRowFull(int row) const
{
    for (BlockType cell : m_cells[row]) {
        if (cell == BlockType::Empty) {
            return false;
        }
    }
    return true;
}
