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
// OpenGL board rendering so it draws on top of it.
void drawMatchHud(const HudPlayerStats& player0, const HudPlayerStats& player1, float windowWidth);

// Draws the game-over overlay: a winner announcement plus rematch/menu
// prompt, on top of the frozen boards behind it. Returns true if the
// player asked to return to the main menu.
bool drawGameOverOverlay(const std::string& winnerName, float windowWidth, float windowHeight);
