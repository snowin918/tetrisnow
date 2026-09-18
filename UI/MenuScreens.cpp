#include "UI/MenuScreens.h"
#include "UI/IceWidgets.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>

#include <imgui.h>

namespace
{
constexpr float kPanelWidth = 360.0f;

// Opens a fixed-width, chrome-free, auto-height panel at the given
// (pos, pivot) — every screen in this file is just one of these with
// different contents. Height auto-sizes so a variable-length status/error
// line never gets clipped. A translucent frosted-glass tint plus a soft
// cyan border distinguishes it from a plain default ImGui window without
// needing any art of its own.
void beginCenteredPanel(const char* name, ImVec2 pos, ImVec2 pivot)
{
    ImGui::SetNextWindowPos(pos, ImGuiCond_Always, pivot);
    ImGui::SetNextWindowSize(ImVec2(kPanelWidth, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.10f, 0.16f, 0.72f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.45f, 0.75f, 0.95f, 0.55f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 14.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(28.0f, 24.0f));
    ImGui::Begin(
        name, nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
}

// Pops the style pushed by beginCenteredPanel() — always call this instead
// of a bare ImGui::End() for a panel opened that way, so the push/pop
// counts stay balanced.
void endCenteredPanel()
{
    ImGui::End();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}

void drawTitle(const char* title, const char* subtitle = nullptr)
{
    const float textWidth = ImGui::CalcTextSize(title).x;
    ImGui::SetCursorPosX((kPanelWidth - textWidth) * 0.5f);
    ImGui::TextColored(ImVec4(0.80f, 0.92f, 1.0f, 1.0f), "%s", title);
    if (subtitle != nullptr) {
        const float subWidth = ImGui::CalcTextSize(subtitle).x;
        ImGui::SetCursorPosX((kPanelWidth - subWidth) * 0.5f);
        ImGui::TextColored(ImVec4(0.55f, 0.68f, 0.80f, 0.85f), "%s", subtitle);
    }
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

bool centeredButton(const char* label, float width = 260.0f)
{
    ImGui::SetCursorPosX((kPanelWidth - width) * 0.5f);
    return iceButton(label, ImVec2(width, 42.0f));
}

// A borderless, input-transparent window pinned to one screen edge,
// cycling through all 16 poses of a 4x4-pose character sprite sheet — the
// same portraits used nowhere else in the UI, purely decorative framing
// for the title screen. pivot (0, 1) anchors pos to the window's
// bottom-left corner (for the left edge); (1, 1) anchors to the
// bottom-right (for the right edge), so pos can just be the screen's
// bottom corner and the portrait stays flush against it.
void drawSidePortrait(
    ImTextureID texture, ImVec2 pos, ImVec2 pivot, float height, float animationSeconds)
{
    if (texture == nullptr) {
        return;
    }
    const float width = height; // The source atlas contains square cells.
    const ImVec2 corner(pos.x - width * pivot.x, pos.y - height * pivot.y);

    constexpr float kFrameDuration = 0.15f;
    constexpr int kFrameCount = 16; // full 4x4 grid, read in row-major order
    const float t = std::fmod(std::max(animationSeconds, 0.0f), kFrameDuration * kFrameCount);
    const int frame = std::clamp(static_cast<int>(t / kFrameDuration), 0, kFrameCount - 1);
    const ImVec2 uv0((frame % 4) * 0.25f, (frame / 4) * 0.25f);
    const ImVec2 uv1(uv0.x + 0.25f, uv0.y + 0.25f);
    ImGui::GetBackgroundDrawList()->AddImage(texture, corner,
        ImVec2(corner.x + width, corner.y + height), uv0, uv1);
}

// The Assets/title.png logo, top-center, sized off its own aspect ratio
// (it's a wide lockup, not a square icon) so it never looks stretched.
void drawTitleLogo(ImTextureID texture, float windowWidth, float topY, float width, float seconds)
{
    if (texture == nullptr) {
        return;
    }
    constexpr float kLogoAspect = 1774.0f / 887.0f; // Assets/title.png's native size
    width *= 1.0f + 0.012f * std::sin(seconds * 1.3f);
    const float height = width / kLogoAspect;
    const ImVec2 p(windowWidth * 0.5f - width * 0.5f, topY + 4.0f * std::sin(seconds * 1.7f));
    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    draw->AddImage(texture, p, ImVec2(p.x + width, p.y + height));
    for (int i = 0; i < 7; ++i) {
        const float pulse = std::pow(std::max(0.0f, std::sin(seconds * 2.0f + i * 2.3f)), 10.0f);
        const ImVec2 star(p.x + width * (0.12f + i * 0.125f),
            p.y + height * (0.35f + 0.14f * std::sin(i * 4.0f)));
        const ImU32 color = IM_COL32(225, 255, 255, static_cast<int>(pulse * 230));
        draw->AddLine(ImVec2(star.x - 5 * pulse, star.y), ImVec2(star.x + 5 * pulse, star.y), color, 1.5f);
        draw->AddLine(ImVec2(star.x, star.y - 8 * pulse), ImVec2(star.x, star.y + 8 * pulse), color, 1.5f);
    }
}
} // namespace

void drawMenuBackground(float width, float height, unsigned int texture, float seconds)
{
    if (width <= 0 || height <= 0) return;
    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    if (texture != 0) {
        // Cover the viewport without stretching the landscape.
        constexpr float aspect = 16.0f / 9.0f;
        const float imageWidth = std::max(width, height * aspect);
        const float imageHeight = imageWidth / aspect;
        const ImVec2 uv((1.0f - width / imageWidth) * 0.5f, (1.0f - height / imageHeight) * 0.5f);
        draw->AddImage(reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(texture)),
            ImVec2(0, 0), ImVec2(width, height), uv, ImVec2(1 - uv.x, 1 - uv.y));
    }
    draw->AddRectFilledMultiColor(ImVec2(0, 0), ImVec2(width, height),
        IM_COL32(9, 23, 33, 65), IM_COL32(9, 23, 33, 65),
        IM_COL32(14, 30, 40, 85), IM_COL32(14, 30, 40, 85));
    // Deterministic layers give the storm depth without per-frame particle allocation.
    const int count = std::clamp(static_cast<int>(width * height / 2600.0f), 100, 650);
    for (int i = 0; i < count; ++i) {
        const float seed = std::fmod(std::sin(i * 127.1f + 3.0f) * 43758.5453f, 1.0f) + 1.0f;
        const float depth = 0.2f + (i % 7) * 0.13f;
        const float speed = 30.0f + depth * 105.0f;
        const float drift = seconds * speed + depth * (65.0f * std::sin(seconds * 0.6f) + 12.0f * std::sin(seconds * 1.7f));
        const float x = std::fmod(i * 137.31f + drift + 12.0f * std::sin(seconds + seed * 20), width + 80.0f) - 40.0f;
        const float y = std::fmod(i * 79.73f + seconds * speed * (0.7f + seed * 0.15f), height + 60.0f) - 30.0f;
        const int alpha = static_cast<int>(45 + depth * 125);
        if (i % 4 == 0) {
            draw->AddLine(ImVec2(x-3*depth, y-2*depth), ImVec2(x+3*depth,y+2*depth), IM_COL32(230,248,255,alpha/2), depth);
        }
        draw->AddCircleFilled(ImVec2(x,y), 0.6f + depth * 1.4f, IM_COL32(238,248,255,alpha), 7);
        if (depth > 0.8f)
            draw->AddCircleFilled(ImVec2(x,y), 2.6f, IM_COL32(220,241,255,18), 9);
    }
}

MenuResult drawMainMenu(
    float windowWidth, float windowHeight, unsigned int portraitBoy, unsigned int portraitGirl,
    unsigned int titleLogo, float animationSeconds)
{
    MenuResult result;

    // Portraits stand at the screen's bottom-left/bottom-right, like the
    // battle characters framing the arena in-match, sized off screen
    // height so they scale sensibly with the window. The girl's cycle is
    // phase-shifted from the boy's so the two don't visibly step in
    // lockstep.
    const float portraitHeight = std::min(windowHeight * 0.64f, windowWidth * 0.46f);
    drawSidePortrait(reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(portraitBoy)),
        ImVec2(-portraitHeight * 0.10f, windowHeight - 12.0f), ImVec2(0.0f, 1.0f), portraitHeight, animationSeconds);
    drawSidePortrait(reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(portraitGirl)),
        ImVec2(windowWidth + portraitHeight * 0.10f, windowHeight - 12.0f), ImVec2(1.0f, 1.0f), portraitHeight, animationSeconds + 0.9f);

    constexpr float kLogoAspect = 1774.0f / 887.0f; // Assets/title.png's native size
    const float kLogoTopY = windowHeight * 0.025f;
    const float logoWidth = std::min({windowWidth * 0.64f, windowHeight * 0.90f, 760.0f});
    drawTitleLogo(reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(titleLogo)), windowWidth, kLogoTopY, logoWidth, animationSeconds);

    // Panel sits just below the logo rather than dead-center, so the two
    // never overlap regardless of the panel's auto-sized height.
    const float panelTopY = kLogoTopY + logoWidth / kLogoAspect + 8.0f;
    const float buttonWidth = std::min(300.0f, windowWidth * 0.43f);
    const float buttonHeight = std::clamp((windowHeight - panelTopY - 60.0f) / 5.0f, 24.0f, 48.0f);
    ImGui::SetNextWindowPos(ImVec2(windowWidth * 0.5f, panelTopY), ImGuiCond_Always, ImVec2(0.5f, 0));
    ImGui::SetNextWindowSize(ImVec2(buttonWidth + 24.0f, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 8));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 10));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.07f, 0.17f, 0.21f, 0.88f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.16f, 0.40f, 0.44f, 0.97f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.11f, 0.31f, 0.35f, 1));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.68f, 0.90f, 0.94f, 0.65f));
    ImGui::Begin("MainMenu", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground
        | ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.74f, 0.92f, 0.94f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.04f, 0.14f, 0.18f, 1));
    if (iceButton("Local Two-Player", ImVec2(buttonWidth, buttonHeight), true)) {
        result.action = MenuAction::StartLocal;
    }
    ImGui::PopStyleColor(2);
    if (iceButton("Host a Match", ImVec2(buttonWidth, buttonHeight))) {
        result.action = MenuAction::GoToHostSetup;
    }
    if (iceButton("Join a Match", ImVec2(buttonWidth, buttonHeight))) {
        result.action = MenuAction::GoToJoinSetup;
    }
    if (iceButton("Quit", ImVec2(buttonWidth, buttonHeight))) {
        result.action = MenuAction::Quit;
    }

    ImGui::End();
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(4);
    return result;
}

MenuResult drawHostSetupScreen(float windowWidth, float windowHeight, bool hosting, const std::string& statusText)
{
    static int portBuffer = 7777;

    MenuResult result;

    beginCenteredPanel("HostSetup", ImVec2(windowWidth * 0.5f, windowHeight * 0.5f), ImVec2(0.5f, 0.5f));
    drawTitle("Host a Match");

    if (!hosting) {
        if (!statusText.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.55f, 1.0f), "%s", statusText.c_str());
            ImGui::Spacing();
        }

        ImGui::Text("Port");
        ImGui::SetNextItemWidth(260.0f);
        ImGui::SetCursorPosX((kPanelWidth - 260.0f) * 0.5f);
        ImGui::InputInt("##port", &portBuffer, 0, 0);
        portBuffer = std::clamp(portBuffer, 1, 65535);

        ImGui::Spacing();
        if (centeredButton("Start Hosting")) {
            result.action = MenuAction::StartHost;
            result.port = static_cast<uint16_t>(portBuffer);
        }
    } else {
        ImGui::TextWrapped("%s", statusText.c_str());
    }

    ImGui::Spacing();
    if (centeredButton("Back")) {
        result.action = MenuAction::Back;
    }

    endCenteredPanel();
    return result;
}

MenuResult drawJoinSetupScreen(float windowWidth, float windowHeight, bool connecting, const std::string& statusText)
{
    static char ipBuffer[64] = "127.0.0.1";
    static int portBuffer = 7777;

    MenuResult result;

    beginCenteredPanel("JoinSetup", ImVec2(windowWidth * 0.5f, windowHeight * 0.5f), ImVec2(0.5f, 0.5f));
    drawTitle("Join a Match");

    if (!connecting) {
        if (!statusText.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.55f, 1.0f), "%s", statusText.c_str());
            ImGui::Spacing();
        }

        ImGui::Text("Host address");
        ImGui::SetNextItemWidth(260.0f);
        ImGui::SetCursorPosX((kPanelWidth - 260.0f) * 0.5f);
        ImGui::InputText("##ip", ipBuffer, sizeof(ipBuffer));

        ImGui::Text("Port");
        ImGui::SetNextItemWidth(260.0f);
        ImGui::SetCursorPosX((kPanelWidth - 260.0f) * 0.5f);
        ImGui::InputInt("##port", &portBuffer, 0, 0);
        portBuffer = std::clamp(portBuffer, 1, 65535);

        ImGui::Spacing();
        if (centeredButton("Connect")) {
            result.action = MenuAction::StartJoin;
            result.hostAddress = ipBuffer;
            result.port = static_cast<uint16_t>(portBuffer);
        }
    } else {
        ImGui::TextWrapped("%s", statusText.c_str());
    }

    ImGui::Spacing();
    if (centeredButton("Back")) {
        result.action = MenuAction::Back;
    }

    endCenteredPanel();
    return result;
}
