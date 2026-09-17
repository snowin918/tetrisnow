#include "Engine/GameWindow.h"

#define GLFW_INCLUDE_NONE // we load GL functions ourselves; don't let GLFW pull in its own headers.
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <algorithm>
#include <cstdio>
#include <utility>

#include "Engine/BlockColors.h"
#include "Game/Board.h"
#include "Network/NetworkSession.h"

namespace
{
constexpr float kBoardGap = 2.0f; // world-space gap, in cells, between the two boards

glm::vec4 colorForAttackType(SnowAttackType type)
{
    switch (type) {
        case SnowAttackType::Snowball: return {0.8f, 0.9f, 1.0f, 0.9f};
        case SnowAttackType::SnowBomb: return {0.5f, 0.75f, 1.0f, 0.95f};
        case SnowAttackType::Avalanche: return {0.9f, 0.97f, 1.0f, 1.0f};
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

GameWindow::GameWindow(int width, int height, const char* title, NetworkConfig networkConfig)
    : m_width(width)
    , m_height(height)
    , m_title(title)
    , m_networkConfig(std::move(networkConfig))
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
        shutdownImGui();
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

    if (!initializeImGui()) {
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
    // doesn't disturb that. In Host mode these same two handlers also
    // relay the event to the network client (see onLinesCleared/
    // onAttackLanded below); in Client mode they're never fired locally —
    // clientHandleHostPacket() calls them directly once it decodes the
    // host's relayed message, reusing the exact same effect code.
    for (int i = 0; i < 2; ++i) {
        m_match.player(i).gameManager().addOnLinesCleared(
            [this, i](const std::vector<Board::ClearedLine>& clearedLines, const std::vector<std::vector<int>>&) {
                onLinesCleared(i, clearedLines);
            });
    }
    m_match.setOnAttackLanded([this](int targetIndex, const SnowAttack& attack) { onAttackLanded(targetIndex, attack); });

    // Every lock changes that player's settled grid, whether or not it
    // cleared a line — that's the host's cue to push a fresh board
    // snapshot to the client. Subscribed unconditionally since the role
    // can still change later (picking Host/Join from the main menu);
    // hostSendBoardSnapshot() itself no-ops unless we're actually hosting.
    for (int i = 0; i < 2; ++i) {
        m_match.player(i).gameManager().addOnPieceLocked([this, i] { hostSendBoardSnapshot(i); });
    }

    if (m_networkConfig.skipMenu) {
        if (!initializeNetwork()) {
            return false;
        }
        switch (m_networkConfig.role) {
            case NetworkRole::Local: m_appState = AppState::InMatch; break;
            case NetworkRole::Host: m_appState = AppState::HostSetup; break;
            case NetworkRole::Client: m_appState = AppState::JoinSetup; break;
        }
    } else {
        m_appState = AppState::MainMenu;
    }

    return true;
}

void GameWindow::run()
{
    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
        pollNetwork();
        updateAppState();

        const double now = glfwGetTime();
        const float deltaTime = static_cast<float>(now - lastTime);
        lastTime = now;

        if (m_appState == AppState::InMatch) {
            if (m_networkConfig.role == NetworkRole::Client) {
                clientSendInputState();
            } else {
                processHeldInput(deltaTime);
                m_match.update(deltaTime);
            }
            checkForGameOver();
        }

        // Ambient snow/particles/camera run in every state — a snowy
        // backdrop behind the menus too, not just in-match.
        updatePieceSmoothing(deltaTime);
        updateAmbientSnow(deltaTime);
        m_particles.update(deltaTime);
        m_camera.update(deltaTime);

        if (m_appState == AppState::InMatch && m_networkConfig.role == NetworkRole::Host) {
            hostBroadcastLiveState();
        }
        if (m_network) {
            m_network->flush();
        }

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

    // Menus are driven entirely by ImGui widgets (mouse clicks, text
    // input) — this handler only ever does gameplay actions, and only
    // once a match is actually running, so a stray keystroke while typing
    // a host IP can never reach a GameManager.
    if (m_appState == AppState::GameOver) {
        if (key == GLFW_KEY_R) {
            requestReset();
        }
        return;
    }
    if (m_appState != AppState::InMatch) {
        return;
    }

    if (m_networkConfig.role == NetworkRole::Client) {
        // A network client never mutates game state directly — it only
        // ever tells the host what its player wants to do. The client
        // always plays "Player 2", so it uses Player 2's keyset.
        switch (key) {
            case GLFW_KEY_UP: clientSendInputAction(Protocol::InputActionType::RotateCW); break;
            case GLFW_KEY_RIGHT_CONTROL: clientSendInputAction(Protocol::InputActionType::HardDrop); break;
            case GLFW_KEY_R: clientSendInputAction(Protocol::InputActionType::ResetRequest); break;
            default: break;
        }
        return;
    }

    GameManager& p1 = m_match.player(0).gameManager();
    GameManager& p2 = m_match.player(1).gameManager();

    // Player 1: WASD cluster, always local. Player 2: arrow cluster when
    // there's no network player 2 (Local mode) — in Host mode player 2's
    // discrete actions instead arrive via hostHandleClientPacket().
    switch (key) {
        case GLFW_KEY_W: p1.rotateClockwise(); break;
        case GLFW_KEY_LEFT_CONTROL: p1.hardDrop(); break;
        case GLFW_KEY_UP:
            if (m_networkConfig.role == NetworkRole::Local) {
                p2.rotateClockwise();
            }
            break;
        case GLFW_KEY_RIGHT_CONTROL:
            if (m_networkConfig.role == NetworkRole::Local) {
                p2.hardDrop();
            }
            break;
        case GLFW_KEY_R: requestReset(); break;
        default: break;
    }
}

void GameWindow::pollHeldKey(
    HeldKeyState& state, bool isDown, float deltaTime, float repeatInterval, GameManager& target,
    void (GameManager::*action)())
{
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

    const bool p1LeftDown = glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS;
    const bool p1RightDown = glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS;
    const bool p1DownDown = glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS;

    // Local mode reads player 2's arrow keys directly, same as always.
    // Host mode instead reads the network client's last-reported
    // held-key state — pollHeldKey() itself doesn't care where "isDown"
    // came from.
    const bool isHost = m_networkConfig.role == NetworkRole::Host;
    const bool p2LeftDown = isHost ? m_remoteInput.left : glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS;
    const bool p2RightDown = isHost ? m_remoteInput.right : glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS;
    const bool p2DownDown = isHost ? m_remoteInput.down : glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS;

    pollHeldKey(m_p1Left, p1LeftDown, deltaTime, kMoveRepeatInterval, p1, &GameManager::moveLeft);
    pollHeldKey(m_p1Right, p1RightDown, deltaTime, kMoveRepeatInterval, p1, &GameManager::moveRight);
    pollHeldKey(m_p1Down, p1DownDown, deltaTime, kSoftDropRepeatInterval, p1, &GameManager::softDrop);

    pollHeldKey(m_p2Left, p2LeftDown, deltaTime, kMoveRepeatInterval, p2, &GameManager::moveLeft);
    pollHeldKey(m_p2Right, p2RightDown, deltaTime, kMoveRepeatInterval, p2, &GameManager::moveRight);
    pollHeldKey(m_p2Down, p2DownDown, deltaTime, kSoftDropRepeatInterval, p2, &GameManager::softDrop);
}

void GameWindow::requestReset()
{
    m_match.reset();
    resetPieceSmoothingState();
    m_gameOverWinnerIndex = -1;
    if (m_appState == AppState::GameOver) {
        m_appState = AppState::InMatch;
    }

    if (m_networkConfig.role == NetworkRole::Host && m_network) {
        m_network->sendReliable(Protocol::encodeMatchReset());
        hostSendBoardSnapshot(0);
        hostSendBoardSnapshot(1);
    }
}

void GameWindow::updatePieceSmoothing(float deltaTime)
{
    auto updateOne = [deltaTime](const BoardView& view, SmoothedVec2& smooth, int& lastGeneration) {
        const glm::vec2 logicalPosition(view.activePiece.position());

        if (view.activePieceGeneration != lastGeneration) {
            // A new piece just spawned — this is not a continuation of
            // the previous piece's movement, so snap instead of easing
            // (otherwise the old piece would appear to slide into the new
            // one's spawn position).
            smooth.snapTo(logicalPosition);
            lastGeneration = view.activePieceGeneration;
        } else {
            smooth.setTarget(logicalPosition);
        }
        smooth.update(deltaTime);
    };

    updateOne(boardView(0), m_p1PieceVisual, m_p1LastPieceGeneration);
    updateOne(boardView(1), m_p2PieceVisual, m_p2LastPieceGeneration);
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

GameWindow::BoardView GameWindow::boardView(int playerIndex)
{
    if (m_networkConfig.role == NetworkRole::Client) {
        return m_remoteView[playerIndex];
    }

    Player& player = m_match.player(playerIndex);
    GameManager& gm = player.gameManager();
    BoardView view;
    for (int row = 0; row < Board::kHeight; ++row) {
        for (int col = 0; col < Board::kWidth; ++col) {
            view.cells[static_cast<size_t>(row)][static_cast<size_t>(col)] = gm.board().cellAt(col, row);
        }
    }
    view.activePiece = gm.activePiece();
    view.activePieceGeneration = gm.activePieceGeneration();
    view.gameOver = gm.isGameOver();
    view.nextPieceType = gm.peekNextType();
    view.score = gm.score().score();
    view.snowEnergy = player.snowEnergy();
    return view;
}

const std::vector<InFlightAttack>& GameWindow::inFlightAttacksView() const
{
    if (m_networkConfig.role == NetworkRole::Client) {
        return m_remoteInFlightAttacks;
    }
    return m_match.inFlightAttacks();
}

void GameWindow::onLinesCleared(int playerIndex, const std::vector<Board::ClearedLine>& clearedLines)
{
    if (m_networkConfig.role == NetworkRole::Host && m_network) {
        Protocol::LinesClearedFxMsg msg;
        msg.playerIndex = playerIndex;
        msg.clearedLines = clearedLines;
        m_network->sendReliable(Protocol::encode(msg));
    }

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
    if (m_networkConfig.role == NetworkRole::Host && m_network) {
        Protocol::AttackLandedFxMsg msg;
        msg.targetPlayerIndex = targetPlayerIndex;
        msg.attack = attack;
        m_network->sendReliable(Protocol::encode(msg));
        hostSendBoardSnapshot(targetPlayerIndex); // the garbage rows just changed this board's grid
    }

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

    // Boards only make sense once a match exists; ambient snow (drawn via
    // m_particles below) runs in every state as a backdrop, menus included.
    if (m_appState == AppState::InMatch || m_appState == AppState::GameOver) {
        const BoardView p1View = boardView(0);
        const BoardView p2View = boardView(1);

        const glm::vec2 p1Offset = m_p1PieceVisual.value() - glm::vec2(p1View.activePiece.position());
        const glm::vec2 p2Offset = m_p2PieceVisual.value() - glm::vec2(p2View.activePiece.position());

        drawSingleBoard(boardOriginX(0), p1View, p1Offset);
        drawSingleBoard(boardOriginX(1), p2View, p2Offset);
        drawInFlightAttacks();
    }
    m_particles.draw(m_renderer);

    m_renderer.endFrame();

    renderImGuiFrame();
}

void GameWindow::drawSingleBoard(float originX, const BoardView& view, glm::vec2 pieceVisualOffset)
{
    for (int row = 0; row < Board::kHeight; ++row) {
        for (int col = 0; col < Board::kWidth; ++col) {
            const BlockType cell = view.cellAt(col, row);
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

    if (!view.gameOver) {
        const glm::vec4 activeColor = colorForBlockType(view.activePiece.type());
        for (const glm::ivec2& cell : view.activePiece.cells()) {
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
    for (const InFlightAttack& inFlight : inFlightAttacksView()) {
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
            glm::vec2(x - size / 2.0f, y - size / 2.0f), glm::vec2(size), colorForAttackType(inFlight.attack.type));

        // A trailing sparkle of particles so the projectile reads as more
        // than a bare moving square.
        ParticleSystem::EmitParams trail;
        trail.position = glm::vec2(x, y);
        trail.velocityMin = glm::vec2(-0.5f, -0.5f);
        trail.velocityMax = glm::vec2(0.5f, 0.5f);
        trail.color = colorForAttackType(inFlight.attack.type);
        trail.sizeMin = 0.06f;
        trail.sizeMax = 0.14f;
        trail.lifetimeMin = 0.15f;
        trail.lifetimeMax = 0.3f;
        trail.gravity = 0.0f;
        m_particles.emit(trail, 2);
    }
}

bool GameWindow::initializeNetwork()
{
    // Only used for the CLI --host/--join fast path (skipMenu), which
    // already knows its role/address/port. The menu-driven path instead
    // goes through startHosting()/startJoining(), which create the
    // session the same way but on a button click.
    if (m_networkConfig.role == NetworkRole::Host) {
        m_network = NetworkSession::createHost(m_networkConfig.port);
        if (!m_network) {
            std::fprintf(stderr, "Failed to host on port %u (already in use?)\n", m_networkConfig.port);
            return false;
        }
        m_networkStatusText = "Hosting on port " + std::to_string(m_networkConfig.port) + " -- waiting for a challenger...";
    } else if (m_networkConfig.role == NetworkRole::Client) {
        m_network = NetworkSession::createClient(m_networkConfig.hostAddress, m_networkConfig.port);
        if (!m_network) {
            std::fprintf(stderr, "Could not resolve host '%s'\n", m_networkConfig.hostAddress.c_str());
            return false;
        }
        m_networkStatusText =
            "Connecting to " + m_networkConfig.hostAddress + ":" + std::to_string(m_networkConfig.port) + "...";
    }

    wireNetworkCallbacks();
    return true;
}

void GameWindow::wireNetworkCallbacks()
{
    if (!m_network) {
        return;
    }

    if (m_networkConfig.role == NetworkRole::Host) {
        m_network->setOnConnected([this] {
            m_networkStatusText = "A challenger connected!";
            std::fprintf(stderr, "A challenger connected!\n");
        });
        m_network->setOnDisconnected([this] {
            m_networkStatusText = "Opponent disconnected.";
            std::fprintf(stderr, "Opponent disconnected.\n");
        });
        m_network->setOnPacket([this](const std::vector<uint8_t>& bytes) { hostHandleClientPacket(bytes); });
    } else if (m_networkConfig.role == NetworkRole::Client) {
        m_network->setOnConnected([this] {
            m_networkStatusText = "Connected!";
            std::fprintf(stderr, "Connected!\n");
        });
        m_network->setOnDisconnected([this] {
            m_networkStatusText = "Disconnected from host.";
            std::fprintf(stderr, "Disconnected from host.\n");
        });
        m_network->setOnPacket([this](const std::vector<uint8_t>& bytes) { clientHandleHostPacket(bytes); });
    }
}

void GameWindow::pollNetwork()
{
    if (m_network) {
        m_network->poll();
    }
}

void GameWindow::hostBroadcastLiveState()
{
    if (!m_network || !m_network->isConnected()) {
        return;
    }

    Protocol::LiveStateMsg msg;
    for (int i = 0; i < 2; ++i) {
        Player& player = m_match.player(i);
        GameManager& gm = player.gameManager();
        Protocol::PieceStateMsg& p = msg.players[static_cast<size_t>(i)];
        p.gameOver = gm.isGameOver();
        p.type = gm.activePiece().type();
        p.position = gm.activePiece().position();
        p.rotationState = gm.activePiece().rotationState();
        p.generation = gm.activePieceGeneration();
        p.nextType = gm.peekNextType();
        p.score = gm.score().score();
        p.snowEnergy = player.snowEnergy();
    }

    for (const InFlightAttack& a : m_match.inFlightAttacks()) {
        Protocol::InFlightAttackMsg attackMsg;
        attackMsg.type = a.attack.type;
        attackMsg.power = a.attack.power;
        attackMsg.sourceLinesCleared = a.attack.sourceLinesCleared;
        attackMsg.targetPlayerIndex = a.targetPlayerIndex;
        attackMsg.elapsedSeconds = a.elapsedSeconds;
        attackMsg.durationSeconds = a.durationSeconds;
        msg.inFlightAttacks.push_back(attackMsg);
    }

    m_network->sendUnreliable(Protocol::encode(msg));
}

void GameWindow::hostSendBoardSnapshot(int playerIndex)
{
    if (m_networkConfig.role != NetworkRole::Host || !m_network) {
        return;
    }

    Protocol::BoardSnapshotMsg msg;
    msg.playerIndex = playerIndex;
    const Board& board = m_match.player(playerIndex).gameManager().board();
    for (int row = 0; row < Board::kHeight; ++row) {
        for (int col = 0; col < Board::kWidth; ++col) {
            msg.cells[static_cast<size_t>(row)][static_cast<size_t>(col)] = board.cellAt(col, row);
        }
    }
    m_network->sendReliable(Protocol::encode(msg));
}

void GameWindow::hostHandleClientPacket(const std::vector<uint8_t>& bytes)
{
    if (bytes.empty()) {
        return;
    }

    switch (Protocol::peekType(bytes)) {
        case Protocol::MessageType::InputState:
            m_remoteInput = Protocol::decodeInputState(bytes);
            break;
        case Protocol::MessageType::InputAction: {
            const Protocol::InputActionMsg msg = Protocol::decodeInputAction(bytes);
            GameManager& p2 = m_match.player(1).gameManager();
            switch (msg.action) {
                case Protocol::InputActionType::RotateCW: p2.rotateClockwise(); break;
                case Protocol::InputActionType::HardDrop: p2.hardDrop(); break;
                case Protocol::InputActionType::ResetRequest: requestReset(); break;
            }
            break;
        }
        default:
            break; // not a message the host expects from a client
    }
}

void GameWindow::clientSendInputState()
{
    if (!m_network) {
        return;
    }

    Protocol::InputStateMsg msg;
    msg.left = glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS;
    msg.right = glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS;
    msg.down = glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS;
    m_network->sendUnreliable(Protocol::encode(msg));
}

void GameWindow::clientSendInputAction(Protocol::InputActionType action)
{
    if (!m_network) {
        return;
    }

    Protocol::InputActionMsg msg;
    msg.action = action;
    m_network->sendReliable(Protocol::encode(msg));
}

void GameWindow::clientHandleHostPacket(const std::vector<uint8_t>& bytes)
{
    if (bytes.empty()) {
        return;
    }

    switch (Protocol::peekType(bytes)) {
        case Protocol::MessageType::BoardSnapshot: {
            const Protocol::BoardSnapshotMsg msg = Protocol::decodeBoardSnapshot(bytes);
            m_remoteView[static_cast<size_t>(msg.playerIndex)].cells = msg.cells;
            break;
        }
        case Protocol::MessageType::LiveState: {
            const Protocol::LiveStateMsg msg = Protocol::decodeLiveState(bytes);
            for (int i = 0; i < 2; ++i) {
                const Protocol::PieceStateMsg& p = msg.players[static_cast<size_t>(i)];
                BoardView& view = m_remoteView[static_cast<size_t>(i)];
                view.activePiece = Tetromino(p.type, p.position);
                view.activePiece.setRotationState(p.rotationState);
                view.activePieceGeneration = p.generation;
                view.gameOver = p.gameOver;
                view.nextPieceType = p.nextType;
                view.score = p.score;
                view.snowEnergy = p.snowEnergy;
            }

            m_remoteInFlightAttacks.clear();
            for (const Protocol::InFlightAttackMsg& a : msg.inFlightAttacks) {
                InFlightAttack inFlight;
                inFlight.attack = SnowAttack{a.type, a.power, a.sourceLinesCleared};
                inFlight.targetPlayerIndex = a.targetPlayerIndex;
                inFlight.elapsedSeconds = a.elapsedSeconds;
                inFlight.durationSeconds = a.durationSeconds;
                m_remoteInFlightAttacks.push_back(inFlight);
            }
            break;
        }
        case Protocol::MessageType::LinesClearedFx: {
            const Protocol::LinesClearedFxMsg msg = Protocol::decodeLinesClearedFx(bytes);
            onLinesCleared(msg.playerIndex, msg.clearedLines);
            break;
        }
        case Protocol::MessageType::AttackLandedFx: {
            const Protocol::AttackLandedFxMsg msg = Protocol::decodeAttackLandedFx(bytes);
            onAttackLanded(msg.targetPlayerIndex, msg.attack);
            break;
        }
        case Protocol::MessageType::MatchReset:
            resetPieceSmoothingState();
            m_gameOverWinnerIndex = -1;
            if (m_appState == AppState::GameOver) {
                m_appState = AppState::InMatch;
            }
            break;
        default:
            break; // not a message the client expects from the host
    }
}

bool GameWindow::initializeImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    // install_callbacks=true chains onto whatever GLFW callbacks are
    // already set — our own key/framebuffer-size callbacks are installed
    // above, before this call, so both keep working.
    if (!ImGui_ImplGlfw_InitForOpenGL(m_window, true)) {
        std::fprintf(stderr, "GameWindow: ImGui_ImplGlfw_InitForOpenGL failed\n");
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init("#version 330")) {
        std::fprintf(stderr, "GameWindow: ImGui_ImplOpenGL3_Init failed\n");
        return false;
    }

    m_imguiInitialized = true;
    return true;
}

void GameWindow::shutdownImGui()
{
    if (!m_imguiInitialized) {
        return;
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    m_imguiInitialized = false;
}

void GameWindow::renderImGuiFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    const float width = static_cast<float>(m_width);
    const float height = static_cast<float>(m_height);

    switch (m_appState) {
        case AppState::MainMenu:
            handleMenuResult(drawMainMenu(width, height));
            break;
        case AppState::HostSetup:
            handleMenuResult(drawHostSetupScreen(width, height, m_network != nullptr, m_networkStatusText));
            break;
        case AppState::JoinSetup:
            handleMenuResult(drawJoinSetupScreen(width, height, m_network != nullptr, m_networkStatusText));
            break;
        case AppState::InMatch:
            drawMatchHud(buildHudStats(0), buildHudStats(1), width);
            break;
        case AppState::GameOver: {
            drawMatchHud(buildHudStats(0), buildHudStats(1), width);
            const std::string winnerName =
                m_gameOverWinnerIndex >= 0 ? m_match.player(m_gameOverWinnerIndex).name() : "Nobody";
            if (drawGameOverOverlay(winnerName, width, height)) {
                returnToMainMenu();
            }
            break;
        }
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GameWindow::handleMenuResult(const MenuResult& result)
{
    switch (result.action) {
        case MenuAction::None: break;
        case MenuAction::StartLocal: startLocalMatch(); break;
        case MenuAction::GoToHostSetup:
            m_networkStatusText.clear();
            m_appState = AppState::HostSetup;
            break;
        case MenuAction::GoToJoinSetup:
            m_networkStatusText.clear();
            m_appState = AppState::JoinSetup;
            break;
        case MenuAction::StartHost: startHosting(result.port); break;
        case MenuAction::StartJoin: startJoining(result.hostAddress, result.port); break;
        case MenuAction::Back: returnToMainMenu(); break;
        case MenuAction::Quit: glfwSetWindowShouldClose(m_window, GLFW_TRUE); break;
    }
}

void GameWindow::startLocalMatch()
{
    m_networkConfig.role = NetworkRole::Local;
    m_network.reset();
    m_match.reset();
    resetPieceSmoothingState();
    m_gameOverWinnerIndex = -1;
    m_appState = AppState::InMatch;
}

void GameWindow::startHosting(uint16_t port)
{
    m_network = NetworkSession::createHost(port);
    if (!m_network) {
        m_networkStatusText = "Failed to host on port " + std::to_string(port) + " (already in use?)";
        return; // stay on HostSetup; the form reappears alongside the error above
    }

    m_networkConfig.role = NetworkRole::Host;
    m_networkConfig.port = port;
    m_networkStatusText = "Hosting on port " + std::to_string(port) + " -- waiting for a challenger...";
    wireNetworkCallbacks();
}

void GameWindow::startJoining(const std::string& hostAddress, uint16_t port)
{
    m_network = NetworkSession::createClient(hostAddress, port);
    if (!m_network) {
        m_networkStatusText = "Could not resolve '" + hostAddress + "'";
        return;
    }

    m_networkConfig.role = NetworkRole::Client;
    m_networkConfig.hostAddress = hostAddress;
    m_networkConfig.port = port;
    m_networkStatusText = "Connecting to " + hostAddress + ":" + std::to_string(port) + "...";
    wireNetworkCallbacks();
}

void GameWindow::returnToMainMenu()
{
    m_network.reset();
    m_networkConfig.role = NetworkRole::Local;
    m_networkStatusText.clear();
    m_match.reset();
    resetPieceSmoothingState();
    m_gameOverWinnerIndex = -1;
    m_appState = AppState::MainMenu;
}

void GameWindow::updateAppState()
{
    // Once the host/client link actually connects, leave the setup screen
    // and start the match both sides now agree is beginning.
    if ((m_appState == AppState::HostSetup || m_appState == AppState::JoinSetup) && m_network
        && m_network->isConnected()) {
        m_match.reset();
        resetPieceSmoothingState();
        m_gameOverWinnerIndex = -1;
        m_appState = AppState::InMatch;
    }
}

void GameWindow::checkForGameOver()
{
    const bool p0Over = boardView(0).gameOver;
    const bool p1Over = boardView(1).gameOver;
    if (p0Over || p1Over) {
        m_gameOverWinnerIndex = p0Over && p1Over ? -1 : (p0Over ? 1 : 0);
        m_appState = AppState::GameOver;
    }
}

void GameWindow::resetPieceSmoothingState()
{
    m_p1LastPieceGeneration = -1;
    m_p2LastPieceGeneration = -1;
    m_remoteView[0] = BoardView{};
    m_remoteView[1] = BoardView{};
    m_remoteInFlightAttacks.clear();
}

HudPlayerStats GameWindow::buildHudStats(int playerIndex)
{
    const BoardView view = boardView(playerIndex);
    HudPlayerStats stats;
    stats.name = m_match.player(playerIndex).name();
    stats.score = view.score;
    stats.snowEnergy = view.snowEnergy;
    stats.nextPieceType = view.nextPieceType;
    return stats;
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
