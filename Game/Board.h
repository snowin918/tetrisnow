#pragma once

#include <array>
#include <vector>

#include <glm/glm.hpp>

#include "Game/BlockType.h"

// A single player's Tetris grid: kWidth x kHeight cells, each either empty
// or holding the type of the piece that locked into it.
//
// Board knows nothing about the currently-falling piece — it only tracks
// locked cells, bounds/occupancy, and line clears. GameManager owns the
// active Tetromino and asks Board whether a candidate position is legal
// before committing a move.
class Board
{
public:
    static constexpr int kWidth = 10;
    static constexpr int kHeight = 20;

    // A row that clearFullLines() removed, captured before removal: its
    // original row index and the block types it held. Exists so the
    // rendering layer can spawn clear-effect particles at the right
    // position/color — Board itself has no opinion on how a clear looks.
    struct ClearedLine
    {
        int row;
        std::array<BlockType, kWidth> cells;
    };

    Board();

    // True if (col, row) is within the board's columns and above the
    // floor, and not already occupied. Rows above the visible board
    // (row < 0) are always considered in-bounds and empty — the standard
    // hidden "spawn buffer" above a Tetris board.
    bool canPlaceCell(int col, int row) const;

    // Convenience for a full set of candidate cells (a tetromino's 4).
    bool canPlaceCells(const std::array<glm::ivec2, 4>& cells) const;

    // BlockType::Empty for out-of-bounds or unoccupied cells.
    BlockType cellAt(int col, int row) const;

    // Writes `type` into every given cell. Cells with row < 0 are ignored
    // (nothing to lock into the hidden buffer).
    void lockCells(const std::array<glm::ivec2, 4>& cells, BlockType type);

    // Removes every fully-filled row, shifting the rows above each cleared
    // row down by one. Returns one ClearedLine per row removed.
    std::vector<ClearedLine> clearFullLines();

    // Inserts one garbage row per entry in gapColumnsPerRow at the bottom
    // of the board, shifting existing rows up. Each garbage row is filled
    // with BlockType::Snow except at that entry's columns, left empty —
    // GameManager shapes these to echo the piece that triggered the
    // attack, so it's not always just a single column. Returns false if
    // this pushed previously-occupied cells above the top of the board —
    // the receiving player's stack has overflowed.
    bool addGarbageRows(const std::vector<std::vector<int>>& gapColumnsPerRow);

    // The row index of the topmost occupied cell across all columns, or
    // kHeight if the board is completely empty. Lower values mean a
    // taller stack (row 0 is the very top) — used as a "how close to
    // topping out" signal (see Game/CharacterController's near-defeat
    // trigger).
    int highestOccupiedRow() const;

    void reset();

private:
    bool isRowFull(int row) const;
    bool isRowEmpty(int row) const;

    std::array<std::array<BlockType, kWidth>, kHeight> m_cells;
};
