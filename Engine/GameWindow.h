#pragma once

#include <glm/glm.hpp>

#include "Engine/Camera.h"
#include "Engine/OpenGLLoader.h"
#include "Engine/Renderer.h"
#include "Game/GameManager.h"

struct GLFWwindow;

// Owns the GLFW window/OpenGL context, drives the game loop, and renders
// the current GameManager state.
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
    void onFramebufferResized(int width, int height);
    void onKey(int key, int action);

    void render();
    void drawBoard();

    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    GLFWwindow* m_window = nullptr;
    int m_width;
    int m_height;
    const char* m_title;

    Camera m_camera;
    Renderer m_renderer;
    GameManager m_gameManager;
};
