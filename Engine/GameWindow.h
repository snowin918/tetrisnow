#pragma once

#include <glm/glm.hpp>

#include "Engine/Camera.h"
#include "Engine/OpenGLLoader.h"
#include "Engine/Renderer.h"
#include "Game/Match.h"

struct GLFWwindow;

// Owns the GLFW window/OpenGL context, drives the game loop, and renders
// the current Match state (both players' boards, plus in-flight snow
// attacks).
//
// With Qt gone, there's no separate OS-level "main window" hosting a
// widget — GameWindow both is the window and runs the loop, which is all
// the structure a single-window desktop game needs.
class GameWindow
{
public:
    GameWindow(int width, int height, const char* title);
    ~GameWindow();

    GameWindow(const GameWindow&) = delete;
    GameWindow& operator=(const GameWindow&) = delete;

    // Creates the window/context and loads OpenGL functions. Returns false
    // on failure (details are printed to stderr).
    bool initialize();

    // Runs the main loop until the window is closed.
    void run();

private:
    // Tracks a held, auto-repeating action (move left/right, soft drop) so
    // its repeat rate is driven by our own game-tied timer rather than the
    // OS's keyboard-repeat setting, which is unreliable for game input —
    // especially with several keys held across two players at once.
    struct HeldKeyState
    {
        bool held = false;
        float timer = 0.0f;
    };

    void onFramebufferResized(int width, int height);
    void onKey(int key, int action);
    void processHeldInput(float deltaTime);
    void pollHeldKey(
        HeldKeyState& state, int glfwKey, float deltaTime, float repeatInterval, GameManager& target,
        void (GameManager::*action)());

    void render();
    void drawSingleBoard(float originX, const GameManager& gameManager);
    void drawInFlightAttacks();

    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    GLFWwindow* m_window = nullptr;
    int m_width;
    int m_height;
    const char* m_title;

    Camera m_camera;
    Renderer m_renderer;
    Match m_match;

    HeldKeyState m_p1Left;
    HeldKeyState m_p1Right;
    HeldKeyState m_p1Down;
    HeldKeyState m_p2Left;
    HeldKeyState m_p2Right;
    HeldKeyState m_p2Down;
};
