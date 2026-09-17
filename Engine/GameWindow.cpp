#include "Engine/GameWindow.h"

#define GLFW_INCLUDE_NONE // we load GL functions ourselves; don't let GLFW pull in its own headers.
#include <GLFW/glfw3.h>

#include <cstdio>

#include "Game/Board.h"

namespace
{
// Maps a piece/block type to its render color. This is deliberately kept
// out of Game/ — BlockType itself carries no color, keeping gameplay
// independent of rendering (Milestone 3 requirement).
glm::vec4 colorForBlockType(BlockType type)
{
    switch (type) {
        case BlockType::I: return {0.2f, 0.85f, 0.9f, 1.0f};
        case BlockType::O: return {0.95f, 0.9f, 0.2f, 1.0f};
        case BlockType::T: return {0.65f, 0.25f, 0.85f, 1.0f};
        case BlockType::S: return {0.3f, 0.85f, 0.3f, 1.0f};
        case BlockType::Z: return {0.9f, 0.25f, 0.25f, 1.0f};
        case BlockType::J: return {0.25f, 0.35f, 0.95f, 1.0f};
        case BlockType::L: return {0.95f, 0.6f, 0.1f, 1.0f};
        case BlockType::Empty: break;
    }
    return {1.0f, 1.0f, 1.0f, 1.0f};
}
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
        // into member teardown (m_renderer destructs right after this
        // body, in reverse declaration order), so its glDelete* calls
        // remain valid.
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
    glfwSetKeyCallback(m_window, &GameWindow::keyCallback);

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

    // Frame the board with a little margin above/below.
    m_camera.setWorldHeight(static_cast<float>(Board::kHeight) + 4.0f);
    m_camera.setPosition(glm::vec2(Board::kWidth / 2.0f, Board::kHeight / 2.0f));

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

        m_gameManager.update(deltaTime);
        render();

        glfwSwapBuffers(m_window);
    }
}

void GameWindow::onFramebufferResized(int width, int height)
{
    glViewport(0, 0, width, height);
    m_camera.setViewportSize(width, height);
}

void GameWindow::onKey(int key, int action)
{
    if (action != GLFW_PRESS && action != GLFW_REPEAT) {
        return;
    }

    switch (key) {
        case GLFW_KEY_LEFT:
            m_gameManager.moveLeft();
            break;
        case GLFW_KEY_RIGHT:
            m_gameManager.moveRight();
            break;
        case GLFW_KEY_DOWN:
            m_gameManager.softDrop();
            break;
        default:
            break;
    }

    // Rotation, hard drop, and reset only respond to the initial press —
    // holding them down shouldn't repeat-fire.
    if (action != GLFW_PRESS) {
        return;
    }

    switch (key) {
        case GLFW_KEY_UP:
            m_gameManager.rotateClockwise();
            break;
        case GLFW_KEY_Z:
            m_gameManager.rotateCounterClockwise();
            break;
        case GLFW_KEY_SPACE:
            m_gameManager.hardDrop();
            break;
        case GLFW_KEY_R:
            m_gameManager.reset();
            break;
        default:
            break;
    }
}

void GameWindow::render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    drawBoard();
}

void GameWindow::drawBoard()
{
    m_renderer.beginFrame(m_camera);

    const Board& board = m_gameManager.board();
    for (int row = 0; row < Board::kHeight; ++row) {
        for (int col = 0; col < Board::kWidth; ++col) {
            const BlockType cell = board.cellAt(col, row);
            const glm::vec2 cellPosition(static_cast<float>(col), static_cast<float>(row));

            if (cell == BlockType::Empty) {
                // Alternating checker background marks empty cells as a
                // visual grid guide.
                const bool alt = (row + col) % 2 == 0;
                const glm::vec4 backgroundColor = alt ? glm::vec4(0.15f, 0.17f, 0.22f, 1.0f)
                                                       : glm::vec4(0.12f, 0.14f, 0.18f, 1.0f);
                m_renderer.drawQuad(cellPosition, glm::vec2(0.95f), backgroundColor);
            } else {
                m_renderer.drawQuad(cellPosition, glm::vec2(1.0f), colorForBlockType(cell));
            }
        }
    }

    const Tetromino& activePiece = m_gameManager.activePiece();
    const glm::vec4 activeColor = colorForBlockType(activePiece.type());
    for (const glm::ivec2& cell : activePiece.cells()) {
        if (cell.y < 0) {
            continue; // still in the hidden spawn buffer above the board
        }
        m_renderer.drawQuad(glm::vec2(static_cast<float>(cell.x), static_cast<float>(cell.y)), glm::vec2(1.0f), activeColor);
    }

    m_renderer.endFrame();
}

void GameWindow::framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    auto* self = static_cast<GameWindow*>(glfwGetWindowUserPointer(window));
    if (self != nullptr) {
        self->onFramebufferResized(width, height);
    }
}

void GameWindow::keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
{
    auto* self = static_cast<GameWindow*>(glfwGetWindowUserPointer(window));
    if (self != nullptr) {
        self->onKey(key, action);
    }
}
