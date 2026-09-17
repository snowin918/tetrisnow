#pragma once

#include <glm/glm.hpp>

#include "Engine/Camera.h"
#include "Engine/OpenGLLoader.h"
#include "Engine/Renderer.h"
#include "Engine/TextureManager.h"

struct GLFWwindow;

// Owns the GLFW window/OpenGL context and drives the game loop.
//
// With Qt gone, there's no separate OS-level "main window" hosting a
// widget — GameWindow both is the window and runs the loop, which is all
// the structure a single-window desktop game needs. Game::GameManager
// (Milestone 3) plugs into tick(); the Renderer (Milestone 2) paints each
// frame via drawTestScene() until real board rendering replaces it.
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

    // Advances game state by deltaTime (seconds). Currently a placeholder;
    // will delegate to Game::GameManager once gameplay logic exists.
    void tick(float deltaTime);

    void render();

    // Milestone 2 proof-of-pipeline scene: a placeholder board grid, a few
    // solid-color blocks, and a textured quad. Replaced by real Board/
    // Tetromino rendering in Milestone 3.
    void drawTestScene();

    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);

    GLFWwindow* m_window = nullptr;
    int m_width;
    int m_height;
    const char* m_title;

    Camera m_camera;
    Renderer m_renderer;
    TextureManager m_textureManager;
    GLuint m_testTexture = 0;
};
