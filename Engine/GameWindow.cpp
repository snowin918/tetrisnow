#include "Engine/GameWindow.h"

#define GLFW_INCLUDE_NONE // we load GL functions ourselves; don't let GLFW pull in its own headers.
#include <GLFW/glfw3.h>

#include <cstdio>

namespace
{
// Standard Tetris board dimensions, used here only to size the Milestone 2
// test scene's camera and grid. Game::Board (Milestone 3) owns these for
// real.
constexpr int kBoardWidthCells = 10;
constexpr int kBoardHeightCells = 20;
} // namespace

GameWindow::GameWindow(int width, int height, const char* title)
    : m_width(width)
    , m_height(height)
    , m_title(title)
{
}

GameWindow::~GameWindow()
{
    if (m_window != nullptr) {
        // Keep the context current through the rest of this destructor and
        // into member teardown (m_renderer, m_textureManager destruct right
        // after this body, in reverse declaration order), so their
        // glDelete* calls remain valid.
        glfwMakeContextCurrent(m_window);
        glfwDestroyWindow(m_window);
    }
    glfwTerminate();
}

bool GameWindow::initialize()
{
    if (!glfwInit()) {
        std::fprintf(stderr, "GameWindow: glfwInit failed\n");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4); // MSAA — smooths block edges.

    m_window = glfwCreateWindow(m_width, m_height, m_title, nullptr, nullptr);
    if (m_window == nullptr) {
        std::fprintf(stderr, "GameWindow: glfwCreateWindow failed\n");
        glfwTerminate();
        return false;
    }

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, &GameWindow::framebufferSizeCallback);

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // vsync

    if (!loadOpenGLFunctions()) {
        std::fprintf(stderr, "GameWindow: failed to load required OpenGL functions\n");
        return false;
    }

    glClearColor(0.05f, 0.08f, 0.12f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_renderer.initialize();
    m_testTexture = m_textureManager.createCheckerboard("testCheckerboard", 64, 8);

    // Frame the placeholder board with a little margin above/below.
    m_camera.setWorldHeight(static_cast<float>(kBoardHeightCells) + 4.0f);
    m_camera.setPosition(glm::vec2(kBoardWidthCells / 2.0f, kBoardHeightCells / 2.0f));

    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(m_window, &framebufferWidth, &framebufferHeight);
    onFramebufferResized(framebufferWidth, framebufferHeight);

    return true;
}

void GameWindow::run()
{
    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();

        const double now = glfwGetTime();
        const float deltaTime = static_cast<float>(now - lastTime);
        lastTime = now;

        tick(deltaTime);
        render();

        glfwSwapBuffers(m_window);
    }
}

void GameWindow::onFramebufferResized(int width, int height)
{
    glViewport(0, 0, width, height);
    m_camera.setViewportSize(width, height);
}

void GameWindow::tick(float deltaTime)
{
    (void)deltaTime;
    // Hook for Game::GameManager in a later milestone.
}

void GameWindow::render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    drawTestScene();
}

void GameWindow::drawTestScene()
{
    m_renderer.beginFrame(m_camera);

    // Placeholder board grid — proves the camera/coordinate system lines up
    // with board-cell space ahead of Game::Board existing.
    for (int row = 0; row < kBoardHeightCells; ++row) {
        for (int col = 0; col < kBoardWidthCells; ++col) {
            const bool alt = (row + col) % 2 == 0;
            const glm::vec4 cellColor = alt ? glm::vec4(0.15f, 0.17f, 0.22f, 1.0f)
                                             : glm::vec4(0.12f, 0.14f, 0.18f, 1.0f);
            m_renderer.drawQuad(
                glm::vec2(static_cast<float>(col), static_cast<float>(row)), glm::vec2(0.95f, 0.95f), cellColor);
        }
    }

    // A 2x2 block of solid-color quads, standing in for a tetromino.
    const glm::vec4 blockColor(0.9f, 0.2f, 0.2f, 1.0f);
    m_renderer.drawQuad(glm::vec2(3.0f, 2.0f), glm::vec2(1.0f), blockColor);
    m_renderer.drawQuad(glm::vec2(4.0f, 2.0f), glm::vec2(1.0f), blockColor);
    m_renderer.drawQuad(glm::vec2(3.0f, 3.0f), glm::vec2(1.0f), blockColor);
    m_renderer.drawQuad(glm::vec2(4.0f, 3.0f), glm::vec2(1.0f), blockColor);

    // Textured quad — proves the texture-sampling draw path.
    m_renderer.drawQuad(glm::vec2(6.0f, 15.0f), glm::vec2(3.0f), m_testTexture);

    m_renderer.endFrame();
}

void GameWindow::framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    auto* self = static_cast<GameWindow*>(glfwGetWindowUserPointer(window));
    if (self != nullptr) {
        self->onFramebufferResized(width, height);
    }
}
