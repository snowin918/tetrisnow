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

    // Wire effect hooks: line clears spawn explosion particles (+ a small
    // shake), landed attacks spawn an impact burst (+ a bigger shake).
    // Match already wired its own onLinesCleared listener (to create the
    // SnowAttack); GameManager supports multiple subscribers so this one
    // doesn't disturb that.
    for (int i = 0; i < 2; ++i) {
        m_match.player(i).gameManager().addOnLinesCleared(
            [this, i](const std::vector<Board::ClearedLine>& clearedLines) { onLinesCleared(i, clearedLines); });
    }
    m_match.setOnAttackLanded([this](int targetIndex, const SnowAttack& attack) { onAttackLanded(targetIndex, attack); });

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

        processHeldInput(deltaTime);
        m_match.update(deltaTime);
        updatePieceSmoothing(deltaTime);
        updateAmbientSnow(deltaTime);
        m_particles.update(deltaTime);
        m_camera.update(deltaTime);

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
    // Movement/soft-drop are handled by processHeldInput()'s per-frame
    // polling instead (see HeldKeyState) — relying on the OS's key-repeat
    // events here was unreliable, especially with keys held by both
    // players at once. Only discrete, non-repeating actions are left here.
    if (action != GLFW_PRESS) {
        return;
    }

    GameManager& p1 = m_match.player(0).gameManager();
    GameManager& p2 = m_match.player(1).gameManager();

    // Player 1: arrow cluster. Player 2: WASD. Two local keysets until
    // Milestone 6 replaces the second one with network input.
    switch (key) {
        case GLFW_KEY_UP: p1.rotateClockwise(); break;
        case GLFW_KEY_ENTER: p1.hardDrop(); break;
        case GLFW_KEY_W: p2.rotateClockwise(); break;
        case GLFW_KEY_LEFT_CONTROL: p2.hardDrop(); break;
        case GLFW_KEY_R: m_match.reset(); break;
        default: break;
    }
}

void GameWindow::pollHeldKey(
    HeldKeyState& state, int glfwKey, float deltaTime, float repeatInterval, GameManager& target,
    void (GameManager::*action)())
{
    const bool isDown = glfwGetKey(m_window, glfwKey) == GLFW_PRESS;

    if (!isDown) {
        state.held = false;
        state.timer = 0.0f;
        return;
    }

    if (!state.held) {
        // Just pressed — fire immediately rather than waiting a full
        // repeat interval, so the first move feels instant.
        state.held = true;
        state.timer = 0.0f;
        (target.*action)();
        return;
    }

    state.timer += deltaTime;
    if (state.timer >= repeatInterval) {
        state.timer -= repeatInterval;
        (target.*action)();
    }
}

void GameWindow::processHeldInput(float deltaTime)
{
    constexpr float kMoveRepeatInterval = 0.10f;     // ~10 moves/sec while held
    constexpr float kSoftDropRepeatInterval = 0.05f; // ~20 drops/sec while held

    GameManager& p1 = m_match.player(0).gameManager();
    GameManager& p2 = m_match.player(1).gameManager();

    pollHeldKey(m_p1Left, GLFW_KEY_LEFT, deltaTime, kMoveRepeatInterval, p1, &GameManager::moveLeft);
    pollHeldKey(m_p1Right, GLFW_KEY_RIGHT, deltaTime, kMoveRepeatInterval, p1, &GameManager::moveRight);
    pollHeldKey(m_p1Down, GLFW_KEY_DOWN, deltaTime, kSoftDropRepeatInterval, p1, &GameManager::softDrop);

    pollHeldKey(m_p2Left, GLFW_KEY_A, deltaTime, kMoveRepeatInterval, p2, &GameManager::moveLeft);
    pollHeldKey(m_p2Right, GLFW_KEY_D, deltaTime, kMoveRepeatInterval, p2, &GameManager::moveRight);
    pollHeldKey(m_p2Down, GLFW_KEY_S, deltaTime, kSoftDropRepeatInterval, p2, &GameManager::softDrop);
}

void GameWindow::updatePieceSmoothing(float deltaTime)
{
    auto updateOne = [deltaTime](GameManager& gm, SmoothedVec2& smooth, int& lastGeneration) {
        const int generation = gm.activePieceGeneration();
        const glm::vec2 logicalPosition(gm.activePiece().position());

        if (generation != lastGeneration) {
            // A new piece just spawned — this is not a continuation of
            // the previous piece's movement, so snap instead of easing
            // (otherwise the old piece would appear to slide into the new
            // one's spawn position).
            smooth.snapTo(logicalPosition);
            lastGeneration = generation;
        } else {
            smooth.setTarget(logicalPosition);
        }
        smooth.update(deltaTime);
    };

    updateOne(m_match.player(0).gameManager(), m_p1PieceVisual, m_p1LastPieceGeneration);
    updateOne(m_match.player(1).gameManager(), m_p2PieceVisual, m_p2LastPieceGeneration);
}

void GameWindow::updateAmbientSnow(float deltaTime)
{
    constexpr float kInterval = 0.06f;
    m_ambientSnowTimer += deltaTime;

    const float totalWidth = 2.0f * static_cast<float>(Board::kWidth) + kBoardGap;
    std::uniform_real_distribution<float> xDist(-2.0f, totalWidth + 2.0f);

    while (m_ambientSnowTimer >= kInterval) {
        m_ambientSnowTimer -= kInterval;

        ParticleSystem::EmitParams params;
        params.position = glm::vec2(xDist(m_ambientRng), -2.0f);
        params.velocityMin = glm::vec2(-0.3f, 1.0f);
        params.velocityMax = glm::vec2(0.3f, 2.0f);
        params.color = glm::vec4(0.9f, 0.95f, 1.0f, 0.5f);
        params.sizeMin = 0.06f;
        params.sizeMax = 0.14f;
        params.lifetimeMin = 5.0f;
        params.lifetimeMax = 8.0f;
        params.gravity = 0.0f;
        m_particles.emit(params, 1);
    }
}

void GameWindow::onLinesCleared(int playerIndex, const std::vector<Board::ClearedLine>& clearedLines)
{
    const float originX = boardOriginX(playerIndex);

    for (const Board::ClearedLine& line : clearedLines) {
        for (int col = 0; col < Board::kWidth; ++col) {
            const BlockType type = line.cells[static_cast<size_t>(col)];
            if (type == BlockType::Empty) {
                continue;
            }

            ParticleSystem::EmitParams params;
            params.position = glm::vec2(originX + static_cast<float>(col) + 0.5f, static_cast<float>(line.row) + 0.5f);
            params.velocityMin = glm::vec2(-2.5f, -3.5f);
            params.velocityMax = glm::vec2(2.5f, -0.5f);
            params.color = colorForBlockType(type);
            params.sizeMin = 0.12f;
            params.sizeMax = 0.28f;
            params.lifetimeMin = 0.35f;
            params.lifetimeMax = 0.65f;
            params.gravity = 6.0f;
            m_particles.emit(params, 6);
        }
    }

    m_camera.triggerShake(0.12f * static_cast<float>(clearedLines.size()), 0.2f);
}

void GameWindow::onAttackLanded(int targetPlayerIndex, const SnowAttack& attack)
{
    const float originX = boardOriginX(targetPlayerIndex);
    const float centerX = originX + static_cast<float>(Board::kWidth) / 2.0f;
    const float bottomY = static_cast<float>(Board::kHeight);

    ParticleSystem::EmitParams params;
    params.position = glm::vec2(centerX, bottomY);
    params.velocityMin = glm::vec2(-4.0f, -4.0f);
    params.velocityMax = glm::vec2(4.0f, -1.0f);
    params.color = glm::vec4(0.85f, 0.92f, 1.0f, 1.0f);
    params.sizeMin = 0.15f;
    params.sizeMax = 0.35f;
    params.lifetimeMin = 0.4f;
    params.lifetimeMax = 0.8f;
    params.gravity = 5.0f;
    m_particles.emit(params, 10 + attack.power * 4);

    m_camera.triggerShake(0.2f + 0.12f * static_cast<float>(attack.power), 0.3f);
}

void GameWindow::render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    m_renderer.beginFrame(m_camera);

    const glm::vec2 p1Offset =
        m_p1PieceVisual.value() - glm::vec2(m_match.player(0).gameManager().activePiece().position());
    const glm::vec2 p2Offset =
        m_p2PieceVisual.value() - glm::vec2(m_match.player(1).gameManager().activePiece().position());

    drawSingleBoard(boardOriginX(0), m_match.player(0).gameManager(), p1Offset);
    drawSingleBoard(boardOriginX(1), m_match.player(1).gameManager(), p2Offset);
    drawInFlightAttacks();
    m_particles.draw(m_renderer);

    m_renderer.endFrame();
}

void GameWindow::drawSingleBoard(float originX, const GameManager& gameManager, glm::vec2 pieceVisualOffset)
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
            const glm::vec2 basePosition(originX + static_cast<float>(cell.x), static_cast<float>(cell.y));
            m_renderer.drawQuad(basePosition + pieceVisualOffset, glm::vec2(1.0f), activeColor);
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

        // A trailing sparkle of particles so the projectile reads as more
        // than a bare moving square.
        ParticleSystem::EmitParams trail;
        trail.position = glm::vec2(x, y);
        trail.velocityMin = glm::vec2(-0.5f, -0.5f);
        trail.velocityMax = glm::vec2(0.5f, 0.5f);
        trail.color = colorForAttackTier(inFlight.attack.tier);
        trail.sizeMin = 0.06f;
        trail.sizeMax = 0.14f;
        trail.lifetimeMin = 0.15f;
        trail.lifetimeMax = 0.3f;
        trail.gravity = 0.0f;
        m_particles.emit(trail, 2);
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
