#include "UI/Hud.h"

#include <imgui.h>

#include "Engine/BlockColors.h"

namespace
{
void drawPlayerPanel(const char* id, ImVec2 pos, const HudPlayerStats& stats)
{
    ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.35f);
    ImGui::Begin(
        id, nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing
            | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::TextColored(ImVec4(0.85f, 0.92f, 1.0f, 1.0f), "%s", stats.name.c_str());
    ImGui::Text("Score: %d", stats.score);
    ImGui::Text("Snow Energy: %d", stats.snowEnergy);

    ImGui::Text("Next:");
    ImGui::SameLine();
    const ImVec2 swatchPos = ImGui::GetCursorScreenPos();
    constexpr float kSwatchSize = 18.0f;
    if (stats.nextPieceType != BlockType::Empty) {
        const glm::vec4 color = colorForBlockType(stats.nextPieceType);
        const ImU32 packed = ImGui::ColorConvertFloat4ToU32(ImVec4(color.r, color.g, color.b, color.a));
        ImGui::GetWindowDrawList()->AddRectFilled(
            swatchPos, ImVec2(swatchPos.x + kSwatchSize, swatchPos.y + kSwatchSize), packed);
    }
    ImGui::Dummy(ImVec2(kSwatchSize, kSwatchSize));

    ImGui::End();
}
} // namespace

void drawMatchHud(const HudPlayerStats& player0, const HudPlayerStats& player1, float windowWidth)
{
    constexpr float kMargin = 16.0f;
    drawPlayerPanel("HudPlayer0", ImVec2(kMargin, kMargin), player0);
    drawPlayerPanel("HudPlayer1", ImVec2(windowWidth * 0.5f + kMargin, kMargin), player1);
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
