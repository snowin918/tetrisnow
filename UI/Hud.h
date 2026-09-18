#pragma once

#include <string>

#include "Game/BlockType.h"

// Per-player numbers the in-match HUD overlay shows — plain data, kept
// separate from GameManager/BoardView so this header has no OpenGL
// dependency of its own beyond BlockType (for the next-piece swatch).
struct HudPlayerStats
{
    std::string name;
    int score = 0;
    int snowEnergy = 0;
    BlockType nextPieceType = BlockType::Empty;
};

// Draws the score/snow-energy/next-piece overlay for both players, one
// panel per board. Call every frame during AppState::InMatch, after the
// OpenGL board rendering so it draws on top of it. player0EdgeScreenX/
// player1EdgeScreenX are the screen-space X (pixels) of each player's ice
// castle's outer wall (see GameWindow::castleOuterEdgeX) — player0's panel
// is placed flush to the left of its edge, player1's flush to the right of
// its edge, so each panel sits beside that player's own castle rather than
// centered above the middle of the screen.
void drawMatchHud(
    const HudPlayerStats& player0, const HudPlayerStats& player1, float windowWidth, float windowHeight,
    float player0EdgeScreenX, float player1EdgeScreenX);

// Draws the game-over overlay: a winner announcement plus rematch/menu
// prompt, on top of the frozen boards behind it. Returns true if the
// player asked to return to the main menu.
bool drawGameOverOverlay(const std::string& winnerName, float windowWidth, float windowHeight);
