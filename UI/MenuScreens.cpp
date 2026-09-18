#include "UI/MenuScreens.h"

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
    return ImGui::Button(label, ImVec2(width, 38.0f));
}

// A borderless, input-transparent window pinned to one screen edge,
// cycling through all 16 poses of a 4x4-pose character sprite sheet — the
// same portraits used nowhere else in the UI, purely decorative framing
// for the title screen. pivot (0, 1) anchors pos to the window's
// bottom-left corner (for the left edge); (1, 1) anchors to the
// bottom-right (for the right edge), so pos can just be the screen's
// bottom corner and the portrait stays flush against it.
void drawSidePortrait(
    const char* id, ImTextureID texture, ImVec2 pos, ImVec2 pivot, float height, float animationSeconds)
{
    if (texture == nullptr) {
        return;
    }
    const float width = height * 0.62f; // sheet cells are taller than wide once cropped to one pose
    ImGui::SetNextWindowPos(pos, ImGuiCond_Always, pivot);
    ImGui::SetNextWindowSize(ImVec2(width, height));
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::Begin(
        id, nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing
            | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground);

    constexpr float kFrameDuration = 0.15f;
    constexpr int kFrameCount = 16; // full 4x4 grid, read in row-major order
    const float t = std::fmod(std::max(animationSeconds, 0.0f), kFrameDuration * kFrameCount);
    const int frame = std::clamp(static_cast<int>(t / kFrameDuration), 0, kFrameCount - 1);
    const ImVec2 uv0((frame % 4) * 0.25f, (frame / 4) * 0.25f);
    const ImVec2 uv1(uv0.x + 0.25f, uv0.y + 0.25f);
    ImGui::Image(texture, ImVec2(width, height), uv0, uv1);
    ImGui::End();
}

// The Assets/title.png logo, top-center, sized off its own aspect ratio
// (it's a wide lockup, not a square icon) so it never looks stretched.
void drawTitleLogo(ImTextureID texture, float windowWidth, float topY, float width)
{
    if (texture == nullptr) {
        return;
    }
    constexpr float kLogoAspect = 1774.0f / 887.0f; // Assets/title.png's native size
    const float height = width / kLogoAspect;
    ImGui::SetNextWindowPos(ImVec2(windowWidth * 0.5f, topY), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(width, height));
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::Begin(
        "MenuTitleLogo", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing
            | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground);
    ImGui::Image(texture, ImVec2(width, height));
    ImGui::End();
}
} // namespace

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
    const float portraitHeight = std::min(windowHeight * 0.85f, 620.0f);
    drawSidePortrait("MenuPortraitBoy", reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(portraitBoy)),
        ImVec2(0.0f, windowHeight), ImVec2(0.0f, 1.0f), portraitHeight, animationSeconds);
    drawSidePortrait("MenuPortraitGirl", reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(portraitGirl)),
        ImVec2(windowWidth, windowHeight), ImVec2(1.0f, 1.0f), portraitHeight, animationSeconds + 0.9f);

    constexpr float kLogoAspect = 1774.0f / 887.0f; // Assets/title.png's native size
    constexpr float kLogoTopY = 28.0f;
    const float logoWidth = std::clamp(windowWidth * 0.5f, 360.0f, 620.0f);
    drawTitleLogo(reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(titleLogo)), windowWidth, kLogoTopY, logoWidth);

    // Panel sits just below the logo rather than dead-center, so the two
    // never overlap regardless of the panel's auto-sized height.
    const float panelTopY = kLogoTopY + logoWidth / kLogoAspect + 20.0f;
    beginCenteredPanel("MainMenu", ImVec2(windowWidth * 0.5f, panelTopY), ImVec2(0.5f, 0.0f));
    drawTitle("A winter versus battle");
    ImGui::Spacing();

    if (centeredButton("Local Two-Player")) {
        result.action = MenuAction::StartLocal;
    }
    ImGui::Spacing();
    if (centeredButton("Host a Match")) {
        result.action = MenuAction::GoToHostSetup;
    }
    ImGui::Spacing();
    if (centeredButton("Join a Match")) {
        result.action = MenuAction::GoToJoinSetup;
    }
    ImGui::Spacing();
    ImGui::Spacing();
    if (centeredButton("Quit")) {
        result.action = MenuAction::Quit;
    }

    endCenteredPanel();
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
