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
    const auto cells = previewPiece.cellsAt(glm::ivec2(0, 0), 0);
    glm::ivec2 minimum(4), maximum(-4);
    for (const auto& cell : cells) {
        minimum = glm::min(minimum, cell);
        maximum = glm::max(maximum, cell);
    }
    const glm::vec2 inset = (glm::vec2(4) - glm::vec2(maximum-minimum+glm::ivec2(1))) * 0.5f;
    for (const glm::ivec2& cell : cells) {
        const ImVec2 cellMin(
            origin.x + (cell.x-minimum.x+inset.x) * kPreviewCellSize,
            origin.y + (cell.y-minimum.y+inset.y) * kPreviewCellSize);
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
    constexpr float kBarHeight = 4.0f;
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

// pivot.x=0 anchors pos to the panel's top-left corner (so pos can be a
// castle's outer edge and the panel extends rightward, away from it);
// pivot.x=1 anchors it to the panel's top-right corner instead (so pos can
// be a castle's outer edge on the other side and the panel extends
// leftward). Either way the panel stays flush against that edge
// regardless of its auto-sized width.
void drawPlayerPanel(const char* id, ImVec2 pos, ImVec2 pivot, const HudPlayerStats& stats, bool secondPlayer, float width)
{
    ImGui::SetNextWindowPos(pos, ImGuiCond_Always, pivot);
    ImGui::SetNextWindowSize(ImVec2(width, 86));
    ImGui::SetNextWindowBgAlpha(0.90f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 8));
    ImGui::Begin(
        id, nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing
            | ImGuiWindowFlags_NoInputs);

    const ImVec4 accent = secondPlayer ? ImVec4(1.0f, 0.70f, 0.83f, 1) : ImVec4(0.54f, 0.94f, 0.96f, 1);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 p = ImGui::GetWindowPos();
    draw->AddLine(ImVec2(p.x+8,p.y), ImVec2(p.x+width-8,p.y), ImGui::ColorConvertFloat4ToU32(accent), 2);
    ImGui::TextColored(accent, "%s", secondPlayer ? "JESSICA" : "THOMAS");
    ImGui::SetCursorPos(ImVec2(12, 32));
    ImGui::Text("%d", stats.score);
    ImGui::SameLine();
    ImGui::TextDisabled("score");
    ImGui::SetCursorPos(ImVec2(12, 57));
    ImGui::TextColored(accent, "Snow  %d", stats.snowEnergy);
    ImGui::SetCursorPos(ImVec2(12, 78));
    drawSnowEnergyBar(stats.snowEnergy);
    ImGui::SetCursorPos(ImVec2(width-61, 9));
    ImGui::TextDisabled("NEXT");
    drawNextPiecePreview(ImVec2(p.x+width-57, p.y+38), stats.nextPieceType);

    ImGui::End();
    ImGui::PopStyleVar(2);
}
} // namespace

void drawMatchHud(
    const HudPlayerStats& player0, const HudPlayerStats& player1, float windowWidth, float windowHeight,
    float player0EdgeScreenX, float player1EdgeScreenX)
{
    (void)windowHeight;
    const float panelWidth = std::min(280.0f, windowWidth * 0.39f);
    constexpr float kEdgeMargin = 14.0f; // breathing room between the panel and the castle wall
    constexpr float kScreenMargin = 8.0f; // never let the panel itself run off the window edge

    // Flush beside the castle wall when there's room; otherwise clamped to
    // stay fully on-screen (possibly nudged over the castle's outer edge a
    // little) rather than letting part of the panel clip off past the
    // window border.
    const float player0PosX = std::max(player0EdgeScreenX - kEdgeMargin, panelWidth + kScreenMargin);
    const float player1PosX = std::min(player1EdgeScreenX + kEdgeMargin, windowWidth - panelWidth - kScreenMargin);

    drawPlayerPanel("HudPlayer0", ImVec2(player0PosX, 10), ImVec2(1.0f, 0.0f), player0, false, panelWidth);
    drawPlayerPanel("HudPlayer1", ImVec2(player1PosX, 10), ImVec2(0.0f, 0.0f), player1, true, panelWidth);
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
