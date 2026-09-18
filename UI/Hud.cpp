#include "UI/Hud.h"

#include <algorithm>

#include <imgui.h>

#include "Engine/BlockColors.h"
#include "Game/Tetromino.h"

namespace
{
constexpr float kPreviewCellSize = 8.0f;
constexpr float kPreviewBoxSize = kPreviewCellSize * 4.0f; // pieces live in a 4x4 bounding box

// Draws the piece's actual 4-cell shape (not just a color swatch) inside
// a fixed 4x4-cell box, using its rotation-0 layout — the same shape data
// Board/GameManager use, so this always matches what will really spawn.
void drawNextPiecePreview(ImVec2 origin)
{
    ImGui::GetWindowDrawList()->AddRect(
        origin, ImVec2(origin.x + kPreviewBoxSize, origin.y + kPreviewBoxSize),
        ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 0.15f)));
}

void drawNextPiecePreview(ImVec2 origin, BlockType type)
{
    drawNextPiecePreview(origin);
    if (type == BlockType::Empty) {
        return;
    }

    const Tetromino previewPiece(type, glm::ivec2(0, 0));
    const glm::vec4 color = colorForBlockType(type);
    const ImU32 packed = ImGui::ColorConvertFloat4ToU32(ImVec4(color.r, color.g, color.b, color.a));

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    for (const glm::ivec2& cell : previewPiece.cellsAt(glm::ivec2(0, 0), 0)) {
        const ImVec2 cellMin(
            origin.x + static_cast<float>(cell.x) * kPreviewCellSize,
            origin.y + static_cast<float>(cell.y) * kPreviewCellSize);
        const ImVec2 cellMax(cellMin.x + kPreviewCellSize, cellMin.y + kPreviewCellSize);
        drawList->AddRectFilled(cellMin, cellMax, packed, 2.0f); // slight rounding to echo the board's ice-cube blocks
    }
}

// A thin filled bar (snow energy is uncapped in principle, so this reads
// as "how charged up" via a soft log-ish scale rather than claiming a false
// 0-100% max) — a quick glance shows who's closer to unleashing an attack
// without needing to read the number.
void drawSnowEnergyBar(int snowEnergy)
{
    constexpr float kBarWidth = 168.0f;
    constexpr float kBarHeight = 10.0f;
    constexpr int kVisualCap = 12; // fill reads as "full" around this value

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec2 end(origin.x + kBarWidth, origin.y + kBarHeight);
    drawList->AddRectFilled(
        origin, end, ImGui::ColorConvertFloat4ToU32(ImVec4(0.08f, 0.14f, 0.22f, 0.85f)), kBarHeight * 0.5f);

    const float fillFrac = std::clamp(static_cast<float>(snowEnergy) / static_cast<float>(kVisualCap), 0.0f, 1.0f);
    if (fillFrac > 0.0f) {
        const ImVec2 fillEnd(origin.x + kBarWidth * fillFrac, end.y);
        const ImU32 fillColor =
            ImGui::ColorConvertFloat4ToU32(ImVec4(0.35f, 0.72f, 0.98f, 0.95f));
        drawList->AddRectFilled(origin, fillEnd, fillColor, kBarHeight * 0.5f);
    }
    drawList->AddRect(
        origin, end, ImGui::ColorConvertFloat4ToU32(ImVec4(0.45f, 0.75f, 0.95f, 0.45f)), kBarHeight * 0.5f);
    ImGui::Dummy(ImVec2(kBarWidth, kBarHeight));
}

// pivot (0,0) anchors pos to the panel's top-left corner (the left
// player's usual placement); pivot (1,0) anchors it to the panel's
// top-right corner instead, so pos can be the screen's right edge and
// the panel stays flush against it regardless of its auto-sized width.
void drawPlayerPanel(const char* id, ImVec2 pos, ImVec2 pivot, const HudPlayerStats& stats)
{
    ImGui::SetNextWindowPos(pos, ImGuiCond_Always, pivot);
    ImGui::SetNextWindowBgAlpha(0.45f);
    ImGui::Begin(
        id, nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing
            | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::TextColored(ImVec4(0.55f, 0.85f, 1.0f, 1.0f), "%s", stats.name.c_str());
    ImGui::Separator();
    ImGui::Text("Score");
    ImGui::SameLine(110.0f);
    ImGui::TextColored(ImVec4(0.95f, 0.98f, 1.0f, 1.0f), "%d", stats.score);

    ImGui::Text("Snow Energy");
    ImGui::SameLine(110.0f);
    ImGui::TextColored(ImVec4(0.75f, 0.90f, 1.0f, 1.0f), "%d", stats.snowEnergy);
    drawSnowEnergyBar(stats.snowEnergy);

    ImGui::Spacing();
    ImGui::Text("Next:");
    ImGui::SameLine();
    drawNextPiecePreview(ImGui::GetCursorScreenPos(), stats.nextPieceType);
    ImGui::Dummy(ImVec2(kPreviewBoxSize, kPreviewBoxSize));

    ImGui::End();
}
} // namespace

void drawMatchHud(const HudPlayerStats& player0, const HudPlayerStats& player1, float windowWidth)
{
    constexpr float kMargin = 16.0f;
    drawPlayerPanel("HudPlayer0", ImVec2(kMargin, kMargin), ImVec2(0.0f, 0.0f), player0);
    drawPlayerPanel("HudPlayer1", ImVec2(windowWidth - kMargin, kMargin), ImVec2(1.0f, 0.0f), player1);
}

bool drawGameOverOverlay(const std::string& winnerName, float windowWidth, float windowHeight)
{
    bool returnToMenu = false;

    ImGui::SetNextWindowPos(ImVec2(windowWidth * 0.5f, windowHeight * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(360.0f, 180.0f));
    ImGui::Begin(
        "GameOver", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);

    const std::string title = winnerName + " wins!";
    const float titleWidth = ImGui::CalcTextSize(title.c_str()).x;
    ImGui::SetCursorPosX((360.0f - titleWidth) * 0.5f);
    ImGui::TextColored(ImVec4(0.85f, 0.92f, 1.0f, 1.0f), "%s", title.c_str());
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextWrapped("Press R for a rematch.");
    ImGui::Spacing();

    constexpr float kButtonWidth = 260.0f;
    ImGui::SetCursorPosX((360.0f - kButtonWidth) * 0.5f);
    if (ImGui::Button("Main Menu", ImVec2(kButtonWidth, 0.0f))) {
        returnToMenu = true;
    }

    ImGui::End();
    return returnToMenu;
}
