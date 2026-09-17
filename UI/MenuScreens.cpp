#include "UI/MenuScreens.h"

#include <algorithm>
#include <cstdio>

#include <imgui.h>

namespace
{
constexpr float kPanelWidth = 360.0f;

// Centers a fixed-width, chrome-free, auto-height panel on screen and
// opens it — every screen in this file is just one of these with
// different contents. Height auto-sizes so a variable-length status/error
// line never gets clipped.
void beginCenteredPanel(const char* name, float windowWidth, float windowHeight)
{
    ImGui::SetNextWindowPos(ImVec2(windowWidth * 0.5f, windowHeight * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(kPanelWidth, 0.0f));
    ImGui::Begin(
        name, nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
}

void drawTitle(const char* title)
{
    const float textWidth = ImGui::CalcTextSize(title).x;
    ImGui::SetCursorPosX((kPanelWidth - textWidth) * 0.5f);
    ImGui::TextColored(ImVec4(0.75f, 0.88f, 1.0f, 1.0f), "%s", title);
    ImGui::Separator();
    ImGui::Spacing();
}

bool centeredButton(const char* label, float width = 260.0f)
{
    ImGui::SetCursorPosX((kPanelWidth - width) * 0.5f);
    return ImGui::Button(label, ImVec2(width, 0.0f));
}
} // namespace

MenuResult drawMainMenu(float windowWidth, float windowHeight)
{
    MenuResult result;

    beginCenteredPanel("MainMenu", windowWidth, windowHeight);
    drawTitle("TETRISNOW");
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

    ImGui::End();
    return result;
}

MenuResult drawHostSetupScreen(float windowWidth, float windowHeight, bool hosting, const std::string& statusText)
{
    static int portBuffer = 7777;

    MenuResult result;

    beginCenteredPanel("HostSetup", windowWidth, windowHeight);
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

    ImGui::End();
    return result;
}

MenuResult drawJoinSetupScreen(float windowWidth, float windowHeight, bool connecting, const std::string& statusText)
{
    static char ipBuffer[64] = "127.0.0.1";
    static int portBuffer = 7777;

    MenuResult result;

    beginCenteredPanel("JoinSetup", windowWidth, windowHeight);
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

    ImGui::End();
    return result;
}
