#include "Engine/GameWindow.h"

#define GLFW_INCLUDE_NONE // we load GL functions ourselves; don't let GLFW pull in its own headers.
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstdio>

#include "Game/Board.h"

namespace
{
constexpr float kBoardGap = 2.0f; // world-space gap, in cells, between the two boards

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
        case BlockType::Snow: return {0.75f, 0.82f, 0.9f, 1.0f};
        case BlockType::Empty: break;
    }
    return {1.0f, 1.0f, 1.0f, 1.0f};
}

glm::vec4 colorForAttackTier(SnowAttackTier tier)
{
    switch (tier) {
        case SnowAttackTier::Snowball: return {0.8f, 0.9f, 1.0f, 0.9f};
        case SnowAttackTier::SnowBomb: return {0.5f, 0.75f, 1.0f, 0.95f};
        case SnowAttackTier::Avalanche: return {0.9f, 0.97f, 1.0f, 1.0f};
    }
    return {1.0f, 1.0f, 1.0f, 1.0f};
}

// World-space X of a board's left edge. Board 0 sits at the origin; board
// 1 starts one board-width plus the gap further right.
float boardOriginX(int playerIndex)
{
    return playerIndex == 0 ? 0.0f : static_cast<float>(Board::kWidth) + kBoardGap;
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

    // Frame both boards side by side, with a little margin above/below.
    const float totalWidth = 2.0f * static_cast<float>(Board::kWidth) + kBoardGap;
    m_camera.setWorldHeight(static_cast<float>(Board::kHeight) + 4.0f);
    m_camera.setPosition(glm::vec2(totalWidth / 2.0f, Board::kHeight / 2.0f));

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

        m_match.update(deltaTime);
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

    GameManager& p1 = m_match.player(0).gameManager();
    GameManager& p2 = m_match.player(1).gameManager();

    // Player 1: arrow cluster. Player 2: WASD. Two local keysets until
    // Milestone 6 replaces the second one with network input.
    switch (key) {
        case GLFW_KEY_LEFT: p1.moveLeft(); break;
        case GLFW_KEY_RIGHT: p1.moveRight(); break;
        case GLFW_KEY_DOWN: p1.softDrop(); break;
        case GLFW_KEY_A: p2.moveLeft(); break;
        case GLFW_KEY_D: p2.moveRight(); break;
        case GLFW_KEY_S: p2.softDrop(); break;
        default: break;
    }

    // Rotation, hard drop, and reset only respond to the initial press —
    // holding them down shouldn't repeat-fire.
    if (action != GLFW_PRESS) {
        return;
    }

    switch (key) {
        case GLFW_KEY_UP: p1.rotateClockwise(); break;
        case GLFW_KEY_ENTER: p1.hardDrop(); break;
        case GLFW_KEY_W: p2.rotateClockwise(); break;
        case GLFW_KEY_LEFT_CONTROL: p2.hardDrop(); break;
        case GLFW_KEY_R: m_match.reset(); break;
        default: break;
    }
}

void GameWindow::render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    m_renderer.beginFrame(m_camera);

    drawSingleBoard(boardOriginX(0), m_match.player(0).gameManager());
    drawSingleBoard(boardOriginX(1), m_match.player(1).gameManager());
    drawInFlightAttacks();

    m_renderer.endFrame();
}

void GameWindow::drawSingleBoard(float originX, const GameManager& gameManager)
{
    const Board& board = gameManager.board();
    for (int row = 0; row < Board::kHeight; ++row) {
        for (int col = 0; col < Board::kWidth; ++col) {
            const BlockType cell = board.cellAt(col, row);
            const glm::vec2 cellPosition(originX + static_cast<float>(col), static_cast<float>(row));

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

    if (!gameManager.isGameOver()) {
        const Tetromino& activePiece = gameManager.activePiece();
        const glm::vec4 activeColor = colorForBlockType(activePiece.type());
        for (const glm::ivec2& cell : activePiece.cells()) {
            if (cell.y < 0) {
                continue; // still in the hidden spawn buffer above the board
            }
            m_renderer.drawQuad(
                glm::vec2(originX + static_cast<float>(cell.x), static_cast<float>(cell.y)), glm::vec2(1.0f),
                activeColor);
        }
    }
}

void GameWindow::drawInFlightAttacks()
{
    for (const InFlightAttack& inFlight : m_match.inFlightAttacks()) {
        const int sourceIndex = 1 - inFlight.targetPlayerIndex;
        // Attacks travel between the two boards' facing inner edges.
        const float startX = boardOriginX(sourceIndex) + (sourceIndex == 0 ? static_cast<float>(Board::kWidth) : 0.0f);
        const float endX = boardOriginX(inFlight.targetPlayerIndex)
            + (inFlight.targetPlayerIndex == 0 ? static_cast<float>(Board::kWidth) : 0.0f);

        const float t =
            inFlight.durationSeconds > 0.0f ? inFlight.elapsedSeconds / inFlight.durationSeconds : 1.0f;
        const float x = startX + (endX - startX) * std::clamp(t, 0.0f, 1.0f);
        const float y = static_cast<float>(Board::kHeight) / 2.0f;

        const float size = 0.6f + static_cast<float>(inFlight.attack.power) * 0.12f;
        m_renderer.drawQuad(
            glm::vec2(x - size / 2.0f, y - size / 2.0f), glm::vec2(size), colorForAttackTier(inFlight.attack.tier));
    }
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
