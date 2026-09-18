#pragma once

#include <cstdint>
#include <string>

// What the player chose on a menu screen this frame, if anything at all.
// GameWindow reacts to this to change AppState and spin up networking —
// these draw functions never touch NetworkSession/Match themselves.
enum class MenuAction
{
    None,
    StartLocal,    // MainMenu: go straight into a local match
    GoToHostSetup, // MainMenu: navigate to the host-setup screen
    GoToJoinSetup, // MainMenu: navigate to the join-setup screen
    StartHost,     // HostSetup: actually start listening
    StartJoin,     // JoinSetup: actually start connecting
    Back,          // HostSetup/JoinSetup: return to the main menu
    Quit,          // MainMenu: quit the app
};

struct MenuResult
{
    MenuAction action = MenuAction::None;
    uint16_t port = 7777;    // StartHost / StartJoin
    std::string hostAddress; // StartJoin only
};

// Draws the title screen: the Assets/title.png logo above a Local / Host /
// Join / Quit panel, flanked by the boy and girl battle-character
// portraits at the screen's left/right edges, each cycling through its
// full 4x4-pose sprite sheet so they read as idling rather than a frozen
// still. Call once per frame while in the main-menu state; assumes an
// ImGui frame is already active (NewFrame() called, Render() not yet
// called) and a snowy background is already drawn behind it.
// portraitBoy/portraitGirl/titleLogo are OpenGL texture names, plain
// `unsigned int` rather than GLuint so this header doesn't need an OpenGL
// include of its own (0 to skip that image, e.g. if it failed to load).
// animationSeconds should keep increasing frame to frame (e.g. from
// glfwGetTime()) so the portrait cycling animates smoothly.
MenuResult drawMainMenu(
    float windowWidth, float windowHeight, unsigned int portraitBoy, unsigned int portraitGirl,
    unsigned int titleLogo, float animationSeconds);

// Draws the "host a match" screen: a port field and a Start button before
// hosting begins. Once `hosting` is true, `statusText` (e.g. "Waiting for
// a challenger...") replaces the form entirely. Before that, a non-empty
// `statusText` is instead shown inline as an error above the form (e.g.
// "failed to host on port X") without hiding it, so the player can retry.
// A Back button is always available to cancel and return to the main menu.
MenuResult drawHostSetupScreen(float windowWidth, float windowHeight, bool hosting, const std::string& statusText);

// Draws the "join a match" screen: IP + port fields and a Connect button
// before connecting. `statusText` behaves the same as drawHostSetupScreen's
// (inline error before `connecting`, replacing status once true).
MenuResult drawJoinSetupScreen(
    float windowWidth, float windowHeight, bool connecting, const std::string& statusText);
