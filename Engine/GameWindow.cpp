#include "Engine/GameWindow.h"

#define GLFW_INCLUDE_NONE // we load GL functions ourselves; don't let GLFW pull in its own headers.
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <random>
#include <utility>

#include <glm/gtc/constants.hpp>

#include "Engine/BlockColors.h"
#include "Engine/SpriteCharacterAsset.h"
#include "Game/Board.h"
#include "Network/NetworkSession.h"
#include "out/VisualCapture.h"

namespace
{
// World-space gap, in cells, between the two boards — wide enough to read
// as a battle arena between them, with both characters standing in it
// (see GameWindow::drawCharacters()) rather than crowding the boards.
constexpr float kBoardGap = 18.0f;

// Shifts the whole decorative ice castle (towers, foundation, glow seams)
// down relative to the actual playfield, which stays anchored at its real
// grid coordinates — the castle reads as sitting a bit lower/more grounded
// without touching where blocks actually render. Shared with
// drawInFlightAttacks() so a volley's launch point still tracks the
// turret's (now-shifted) position.
constexpr float kCastleDrop = 0.35f;

// Shared ice-tower geometry — used by both drawIceFortress() (to actually
// draw the towers) and castleOuterEdgeX() below (to know where their
// outermost wall ends up, e.g. for placing the HUD beside it) — kept in
// one place so the two can't drift apart.
constexpr float kTowerLeftOffset = 1.42f; // left tower's offset from the board's left edge
constexpr float kTowerRightGap = 0.10f; // right tower's gap from the board's right edge
constexpr float kHeavyBaseWidth = 1.48f; // base tower-base block width before flaring
constexpr float kHeavyFlareExtra = 1.3f; // extra width the heavy foundation flares outward by

// World-space X of the outer face of playerIndex's ice castle (past the
// flared heavy foundation blocks) — the empty snow area starts just past
// this. Mirrors the heavy-foundation-block placement math in
// drawIceFortress() exactly (see kHeavyFlareExtra's doc comment).
float castleOuterEdgeX(int playerIndex)
{
    const float originX = playerIndex == 0 ? 0.0f : static_cast<float>(Board::kWidth) + kBoardGap;
    if (playerIndex == 0) {
        return originX - kTowerLeftOffset - kTowerRightGap - kHeavyFlareExtra;
    }
    // Mirrors drawIceFortress(): the right tower's heavyX ends up at
    // exactly originX+width (the kTowerRightGap used to offset towerX
    // outward and the one subtracted back off for heavyX cancel out).
    return originX + static_cast<float>(Board::kWidth) + kHeavyBaseWidth + kHeavyFlareExtra;
}

// How long a rotation's scale "pop" lasts — short enough to read as a snap
// of feedback rather than a lingering wobble.
constexpr float kRotationPulseDuration = 0.12f;

// How long to ignore key events after regaining window focus — see
// m_inputSuppressRemaining's doc comment in GameWindow.h.
constexpr float kFocusRegainInputSuppressSeconds = 0.2f;

glm::vec4 colorForAttackType(SnowAttackType type)
{
    switch (type) {
        case SnowAttackType::Snowball: return {0.8f, 0.9f, 1.0f, 0.9f};
        case SnowAttackType::SnowBomb: return {0.5f, 0.75f, 1.0f, 0.95f};
        case SnowAttackType::Avalanche: return {0.9f, 0.97f, 1.0f, 1.0f};
    }
    return {1.0f, 1.0f, 1.0f, 1.0f};
}

// A compact volley keeps the board readable; stronger attacks add more snow bullets.
int projectileCountForAttackPower(int power)
{
    constexpr int kMinProjectiles = 5;
    constexpr int kMaxProjectiles = 18;
    return std::clamp(power * 3, kMinProjectiles, kMaxProjectiles);
}

// Individual bullet length (nose-to-tail, along its direction of travel —
// see the missile-style orientation in drawInFlightAttacks()) scales with
// the attack's power directly, not with how many projectiles it split
// into — a capped-count Avalanche still throws visibly chunkier bullets
// than a Snowball attack's, even though both are capped toward similar
// counts.
float bulletLengthForProjectile(int power)
{
    return 1.25f + static_cast<float>(power) * 0.135f;
}

// InFlightAttack::seed is fixed once at the attack's creation (Match::
// onLinesCleared / the Host's network relay carries it in InFlightAttackMsg
// now too — see Protocol.h) rather than re-derived every frame, so every
// bullet's randomized flight shape stays put instead of reshuffling on any
// frame where re-derivation would've drifted (see InFlightAttack::seed's
// doc comment for why that used to flicker). subIndex distinguishes a
// volley's siblings (and -1 for a value shared across the whole volley,
// e.g. the rough landing column every sibling clusters around).
uint32_t seedForProjectile(uint32_t baseSeed, int subIndex)
{
    uint64_t h = 1469598103934665603ull ^ static_cast<uint64_t>(baseSeed); // FNV-1a offset basis
    auto mix = [&h](uint64_t v) {
        h ^= v;
        h *= 1099511628211ull; // FNV-1a prime
    };
    mix(static_cast<uint64_t>(subIndex));
    return static_cast<uint32_t>(h ^ (h >> 32));
}

// One projectile's randomized-but-stable (per seedForProjectile()) flight
// character. Everything here is drawn once per seed, not re-rolled per
// frame, so a given attack's look stays consistent across its flight.
// Used to place a single cubic Bezier's control points (see
// cubicBezier()/drawInFlightAttacks()) rather than switching between
// separate formulas partway through the flight — a real Bezier curve is
// smooth (continuous position *and* direction) everywhere by construction,
// so there's no seam to look like a hitch no matter how it's shaped.
struct ProjectileFlightParams
{
    float arcHeight;          // slight lift/dip early on, board units — kept small; missiles fly flat/direct, not lobbed
    float bounceLift;         // a "guided" course-correction lift right around the wall
    float bounceSwerve;       // lateral course-correction right around the wall
    float midpointFraction;   // where along x the arc's control point sits (not always the exact middle)
    float impactColumnOffset; // spread off the volley's shared landing column
    float lengthJitter;
    float widthJitter;
    // Reparameterizes t before it's used anywhere below (Bezier position)
    // — <1 eases in fast then lingers, >1 lingers then rushes — purely so
    // a volley's bullets don't all move in perfect lockstep despite still
    // all arriving together at t=1.
    float easePower;
};

ProjectileFlightParams makeFlightParams(std::mt19937& rng)
{
    // Deliberately modest compared to the earlier lobbed-snowball version
    // — a missile flies flat and direct, with at most a slight guided
    // curve, not a big parabolic arc.
    std::uniform_real_distribution<float> arcDist(0.2f, 1.2f);
    std::uniform_real_distribution<float> bounceLiftDist(0.2f, 1.6f);
    std::uniform_real_distribution<float> bounceSwerveDist(0.4f, 2.2f);
    std::uniform_real_distribution<float> midpointDist(0.32f, 0.68f);
    std::uniform_real_distribution<float> signDist(-1.0f, 1.0f);
    std::uniform_real_distribution<float> columnOffsetDist(-3.5f, 3.5f);
    std::uniform_real_distribution<float> lengthJitterDist(0.85f, 1.25f);
    std::uniform_real_distribution<float> widthJitterDist(0.85f, 1.15f);
    std::uniform_real_distribution<float> easePowerDist(0.7f, 1.5f);

    ProjectileFlightParams p;
    p.arcHeight = arcDist(rng) * (signDist(rng) < 0.0f ? -1.0f : 1.0f);
    p.bounceLift = bounceLiftDist(rng);
    p.bounceSwerve = bounceSwerveDist(rng) * (signDist(rng) < 0.0f ? -1.0f : 1.0f);
    p.midpointFraction = midpointDist(rng);
    p.impactColumnOffset = columnOffsetDist(rng);
    p.lengthJitter = lengthJitterDist(rng);
    p.widthJitter = widthJitterDist(rng);
    p.easePower = easePowerDist(rng);
    return p;
}

// Standard cubic Bezier: smooth (continuous position and tangent) over the
// whole [0,1] range by construction, no matter how sharply the control
// points are placed — used here instead of a piecewise/branching formula
// so there's no seam where the curve could visibly kink.
glm::vec2 cubicBezier(const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3, float t)
{
    const float u = 1.0f - t;
    return (u * u * u) * p0 + (3.0f * u * u * t) * p1 + (3.0f * u * t * t) * p2 + (t * t * t) * p3;
}

// The cubic Bezier's derivative — its direction of travel at t — used to
// orient each bullet nose-first along its own flight path (missile-style)
// instead of free-spinning. A fast arbitrary spin combined with the round
// shader's off-center highlight glint (see softcircle.frag) swept that
// highlight around fast enough on a small bullet to read as "blinking";
// orienting to the actual (much more gradually turning) travel direction
// removes that entirely as a side effect, on top of just looking right for
// a directed projectile.
glm::vec2 cubicBezierTangent(
    const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3, float t)
{
    const float u = 1.0f - t;
    return (3.0f * u * u) * (p1 - p0) + (6.0f * u * t) * (p2 - p1) + (3.0f * t * t) * (p3 - p2);
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
    , m_effects(m_camera)
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
    glfwSetWindowFocusCallback(m_window, &GameWindow::windowFocusCallback);

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
    m_battlefieldTexture = m_textureManager.loadFromFile(
        "frozen_battlefield", std::string(TETRISNOW_ASSETS_DIR) + "/Backgrounds/FrozenBattlefield.png");
    m_characterAsset =
        std::make_unique<SpriteCharacterAsset>(m_textureManager, std::string(TETRISNOW_ASSETS_DIR) + "/Characters");
    m_menuPortraitBoy = m_textureManager.loadFromFile(
        "menu_portrait_boy", std::string(TETRISNOW_ASSETS_DIR) + "/Characters/Thomas_main.png");
    m_menuPortraitGirl = m_textureManager.loadFromFile(
        "menu_portrait_girl", std::string(TETRISNOW_ASSETS_DIR) + "/Characters/Jessica_main.png");
    m_menuTitleLogo =
        m_textureManager.loadFromFile("menu_title_logo", std::string(TETRISNOW_ASSETS_DIR) + "/title.png");
    m_menuBackground = m_textureManager.loadFromFile(
        "menu_background", std::string(TETRISNOW_ASSETS_DIR) + "/Backgrounds/StormCastles.png");

    // Frame both boards side by side, with a little margin above/below.
    const float totalWidth = 2.0f * static_cast<float>(Board::kWidth) + kBoardGap;
    m_camera.setWorldHeight(static_cast<float>(Board::kHeight) + 8.0f);
    m_camera.setPosition(glm::vec2(totalWidth / 2.0f, Board::kHeight / 2.0f));
    m_effects.setAmbientSnowSpan(-2.0f, totalWidth + 2.0f);

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

        updateInputSuppression(deltaTime);

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
        // backdrop behind the menus too, not just in-match. Menus get a
        // heavier, windier snowfall (see EffectManager::update()'s
        // intensity doc) so the title screen reads as a snowstorm rather
        // than the same gentle in-match drift.
        const bool inMenu = m_appState == AppState::MainMenu || m_appState == AppState::HostSetup
            || m_appState == AppState::JoinSetup;
        updatePieceSmoothing(deltaTime);
        m_effects.update(deltaTime, inMenu ? 0.0f : 1.0f);
        updateCharacters(deltaTime);

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
    const float aspect = static_cast<float>(std::max(width, 1)) / std::max(height, 1);
    m_camera.setWorldHeight(std::max(32.0f, 39.0f / aspect));
}

void GameWindow::onFocusChanged(bool focused)
{
    if (focused) {
        // See m_inputSuppressRemaining's doc comment: a key event landing
        // right on this edge is untrustworthy regardless of source, so
        // briefly ignore all key events rather than trust the first one.
        m_inputSuppressRemaining = kFocusRegainInputSuppressSeconds;
        return;
    }

    // Windows never delivers WM_KEYUP for a key released while this window
    // isn't focused (it goes to whatever window IS focused instead), so a
    // key held at the moment focus was lost could otherwise leave our
    // tracked *KeyDown state — or, previously, glfwGetKey() itself — stuck
    // reporting "still down" forever, with pollHeldKey() then auto-
    // repeating a move nobody is making. Clearing both the raw down-state
    // and the repeat-timer bookkeeping means the next real keydown after
    // refocus is always treated as a fresh press, and a key that's
    // genuinely still held physically just requires one release+press to
    // resume moving — far better than a runaway phantom move.
    m_p1LeftKeyDown = false;
    m_p1RightKeyDown = false;
    m_p1DownKeyDown = false;
    m_p2LeftKeyDown = false;
    m_p2RightKeyDown = false;
    m_p2DownKeyDown = false;

    m_p1Left = HeldKeyState{};
    m_p1Right = HeldKeyState{};
    m_p1Down = HeldKeyState{};
    m_p2Left = HeldKeyState{};
    m_p2Right = HeldKeyState{};
    m_p2Down = HeldKeyState{};
}

void GameWindow::updateInputSuppression(float deltaTime)
{
    if (m_inputSuppressRemaining > 0.0f) {
        m_inputSuppressRemaining = std::max(0.0f, m_inputSuppressRemaining - deltaTime);
    }
}

void GameWindow::onKey(int key, int action)
{
    if (m_inputSuppressRemaining > 0.0f) {
        // Ignore everything, including release events, for a brief window
        // right after regaining focus — see m_inputSuppressRemaining's doc
        // comment. A genuinely-held key just needs one release+press after
        // this window closes to resume, same tradeoff onFocusChanged()
        // already accepts for the focus-loss side of this.
        return;
    }

    // Movement/soft-drop keys are tracked here on every real press/release
    // (see m_p1LeftKeyDown etc.'s doc comment for why this isn't just a
    // glfwGetKey() poll) — GLFW_REPEAT is the OS's own key-repeat, which
    // processHeldInput() already ignores in favor of its own timer, so
    // only PRESS/RELEASE matter for this tracking.
    if (action == GLFW_PRESS || action == GLFW_RELEASE) {
        const bool down = action == GLFW_PRESS;
        // Temporary diagnostic for the "phantom left/right" reports — cheap
        // (only prints on an actual press/release, never per-frame) and
        // safe to leave in; remove once that's confirmed gone for good.
        const char* label = nullptr;
        switch (key) {
            case GLFW_KEY_A: m_p1LeftKeyDown = down; label = "p1 left (A)"; break;
            case GLFW_KEY_D: m_p1RightKeyDown = down; label = "p1 right (D)"; break;
            case GLFW_KEY_S: m_p1DownKeyDown = down; label = "p1 soft-drop (S)"; break;
            case GLFW_KEY_LEFT: m_p2LeftKeyDown = down; label = "p2 left"; break;
            case GLFW_KEY_RIGHT: m_p2RightKeyDown = down; label = "p2 right"; break;
            case GLFW_KEY_DOWN: m_p2DownKeyDown = down; label = "p2 soft-drop"; break;
            default: break;
        }
        if (label != nullptr) {
            std::fprintf(stderr, "[input] %s -> %s\n", label, down ? "DOWN" : "up");
        }
    }

    // Everything below is a discrete, non-repeating action — only ever
    // fires on a genuine press.
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
        case GLFW_KEY_LEFT_CONTROL:
            triggerHardDropVisuals(0, boardView(0));
            p1.hardDrop();
            break;
        case GLFW_KEY_UP:
            if (m_networkConfig.role == NetworkRole::Local) {
                p2.rotateClockwise();
            }
            break;
        case GLFW_KEY_RIGHT_CONTROL:
            if (m_networkConfig.role == NetworkRole::Local) {
                triggerHardDropVisuals(1, boardView(1));
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

    // Left+right simultaneously "down" is never legitimate input for
    // either control scheme — no Tetris move needs both at once. A
    // ghosted/stuck key (common on a shared keyboard once two players'
    // key clusters are pressed together — a real keyboard-matrix
    // limitation, not something GLFW or this app can see through) or, for
    // player 2 on a Host, a stale network report can otherwise leave both
    // permanently true. Left alone, that doesn't move the piece (the two
    // moves cancel out on the board every repeat interval below) but does
    // re-fire both every interval, which reads on screen as the piece
    // continuously "shaking" in place instead of actually sliding, and
    // re-triggers updatePieceSmoothing()'s per-move easing each time.
    // Clearing the combination at its source the moment it's seen means
    // whichever side is genuinely stuck can't linger once the other side
    // releases either — same self-healing spirit as onFocusChanged()'s
    // reset, just for this other way the same two booleans can get wedged.
    if (m_p1LeftKeyDown && m_p1RightKeyDown) {
        std::fprintf(stderr, "[input] p1 left+right both stuck down at once -- clearing both\n");
        m_p1LeftKeyDown = false;
        m_p1RightKeyDown = false;
    }
    const bool isHost = m_networkConfig.role == NetworkRole::Host;
    if (isHost) {
        if (m_remoteInput.left && m_remoteInput.right) {
            std::fprintf(stderr, "[input] p2 (remote) left+right both stuck down at once -- clearing both\n");
            m_remoteInput.left = false;
            m_remoteInput.right = false;
        }
    } else if (m_p2LeftKeyDown && m_p2RightKeyDown) {
        std::fprintf(stderr, "[input] p2 left+right both stuck down at once -- clearing both\n");
        m_p2LeftKeyDown = false;
        m_p2RightKeyDown = false;
    }

    const bool p1LeftDown = m_p1LeftKeyDown;
    const bool p1RightDown = m_p1RightKeyDown;
    const bool p1DownDown = m_p1DownKeyDown;

    // Local mode reads player 2's arrow keys directly, same as always.
    // Host mode instead reads the network client's last-reported
    // held-key state — pollHeldKey() itself doesn't care where "isDown"
    // came from.
    const bool p2LeftDown = isHost ? m_remoteInput.left : m_p2LeftKeyDown;
    const bool p2RightDown = isHost ? m_remoteInput.right : m_p2RightKeyDown;
    const bool p2DownDown = isHost ? m_remoteInput.down : m_p2DownKeyDown;

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
    m_fortressDamage[0] = 0.0f;
    m_fortressDamage[1] = 0.0f;
    for (auto& times : m_fortressBreakTimes) times.fill(0.0);
    m_boardCollapseStarted[0] = 0.0;
    m_boardCollapseStarted[1] = 0.0;
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
    auto updateOne = [this, deltaTime](
                          int playerIndex, const BoardView& view, SmoothedVec2& smooth, int& lastGeneration,
                          int& lastRotationState, float& rotationPulseRemaining) {
        const glm::vec2 logicalPosition(view.activePiece.position());
        const bool isNewPiece = view.activePieceGeneration != lastGeneration;

        if (isNewPiece) {
            // A new piece just spawned — this is not a continuation of
            // the previous piece's movement, so snap instead of easing
            // (otherwise the old piece would appear to slide into the new
            // one's spawn position).
            smooth.snapTo(logicalPosition);
            lastGeneration = view.activePieceGeneration;
        } else {
            smooth.setTarget(logicalPosition);

            // Rotation is an instant cell-layout swap in GameManager — no
            // animation of its own — so a rotation "pop" is detected purely
            // from the rotation state changing between frames for the same
            // piece. This works unmodified for a network Client rendering
            // m_remoteView, since it never runs its own GameManager.
            if (view.activePiece.rotationState() != lastRotationState) {
                rotationPulseRemaining = kRotationPulseDuration;

                glm::vec2 centroid(0.0f);
                for (const glm::ivec2& cell : view.activePiece.cells()) {
                    centroid += glm::vec2(cell);
                }
                centroid /= static_cast<float>(view.activePiece.cells().size());
                const float originX = boardOriginX(playerIndex);
                m_effects.emitRotationPuff(
                    glm::vec2(originX + centroid.x + 0.5f, centroid.y + 0.5f),
                    colorForBlockType(view.activePiece.type()));
            }
        }
        lastRotationState = view.activePiece.rotationState();
        if (rotationPulseRemaining > 0.0f) {
            rotationPulseRemaining = std::max(0.0f, rotationPulseRemaining - deltaTime);
        }

        smooth.update(deltaTime);
    };

    updateOne(
        0, boardView(0), m_p1PieceVisual, m_p1LastPieceGeneration, m_p1LastRotationState,
        m_p1RotationPulseRemaining);
    updateOne(
        1, boardView(1), m_p2PieceVisual, m_p2LastPieceGeneration, m_p2LastRotationState,
        m_p2RotationPulseRemaining);

    auto updateSettle = [deltaTime](SmoothedFloat& offset, std::vector<RowFlash>& flashes) {
        offset.update(deltaTime);

        for (RowFlash& flash : flashes) {
            flash.remaining -= deltaTime;
        }
        flashes.erase(
            std::remove_if(flashes.begin(), flashes.end(), [](const RowFlash& f) { return f.remaining <= 0.0f; }),
            flashes.end());
    };
    updateSettle(m_p1StackSettleOffset, m_p1RowFlashes);
    updateSettle(m_p2StackSettleOffset, m_p2RowFlashes);
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

    if (!clearedLines.empty()) m_characters[playerIndex].onAttackSuccess(static_cast<int>(clearedLines.size()));
    m_effects.spawnBlockClearEffect(boardOriginX(playerIndex), clearedLines);

    // Board has already instantly removed these rows and collapsed the
    // stack down onto them by the time this fires — the settle offset and
    // flashes below are a purely cosmetic approximation of that collapse,
    // layered on top of the already-correct data (see the render-side use
    // in drawSingleBoard()).
    SmoothedFloat& settleOffset = playerIndex == 0 ? m_p1StackSettleOffset : m_p2StackSettleOffset;
    std::vector<RowFlash>& flashes = playerIndex == 0 ? m_p1RowFlashes : m_p2RowFlashes;

    const float lineCount = static_cast<float>(clearedLines.size());
    settleOffset.snapTo(-lineCount);
    settleOffset.setTarget(0.0f);

    for (const Board::ClearedLine& line : clearedLines) {
        flashes.push_back(RowFlash{line.row, 0.15f, 0.15f, glm::vec4(1.0f, 1.0f, 1.0f, 0.9f)});
    }
}

void GameWindow::triggerHardDropVisuals(int playerIndex, const BoardView& beforeDrop)
{
    if (beforeDrop.gameOver) {
        return;
    }

    const std::array<glm::ivec2, 4> startCells = beforeDrop.activePiece.cells();
    glm::ivec2 landingPosition = beforeDrop.activePiece.position();
    while (true) {
        const glm::ivec2 candidate = landingPosition + glm::ivec2(0, 1);
        if (!m_match.player(playerIndex).gameManager().board().canPlaceCells(
                beforeDrop.activePiece.cellsAt(candidate, beforeDrop.activePiece.rotationState()))) {
            break;
        }
        landingPosition = candidate;
    }

    const std::array<glm::ivec2, 4> landingCells =
        beforeDrop.activePiece.cellsAt(landingPosition, beforeDrop.activePiece.rotationState());
    const float originX = boardOriginX(playerIndex);
    const glm::vec4 color = colorForBlockType(beforeDrop.activePiece.type());
    m_effects.spawnHardDropFog(startCells, originX, color);
    m_effects.spawnHardDropImpact(landingCells, originX, color);
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

    // Randomized so the explosion doesn't always land dead-center — an
    // independent draw from the in-flight projectile's own landing column
    // (that function has no stable seed input here, just the bare attack),
    // which is fine: the projectile stops being drawn the instant this
    // fires (Match already removed it), so there's no frame where the two
    // could visibly mismatch.
    std::mt19937 impactRng(std::random_device{}());
    std::uniform_real_distribution<float> impactColumnDist(0.5f, static_cast<float>(Board::kWidth) - 0.5f);
    const float impactX = boardOriginX(targetPlayerIndex) + impactColumnDist(impactRng);
    m_effects.spawnSnowExplosion(impactX, attack.power);
    m_fortressDamage[targetPlayerIndex] =
        std::min(1.0f, m_fortressDamage[targetPlayerIndex] + 0.12f + static_cast<float>(attack.power) * 0.035f);

    // Mirrors onLinesCleared()'s settle/flash approximation: Board has
    // already shifted the whole stack up and filled in the new rows at the
    // bottom by the time this fires, so render everything `power` rows
    // lower than its real position and ease up, reading as new ice rising
    // from below rather than appearing shoved into place.
    SmoothedFloat& settleOffset = targetPlayerIndex == 0 ? m_p1StackSettleOffset : m_p2StackSettleOffset;
    std::vector<RowFlash>& flashes = targetPlayerIndex == 0 ? m_p1RowFlashes : m_p2RowFlashes;

    settleOffset.snapTo(static_cast<float>(attack.power));
    settleOffset.setTarget(0.0f);

    for (int i = 0; i < attack.power; ++i) {
        const int row = Board::kHeight - attack.power + i;
        flashes.push_back(RowFlash{row, 0.2f, 0.2f, glm::vec4(0.75f, 0.88f, 1.0f, 0.85f)});
    }

    m_characters[targetPlayerIndex].onAttackReceived(attack.power);

}

void GameWindow::render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    m_renderer.beginFrame(m_camera);

    // Boards only make sense once a match exists; ambient snow (drawn via
    // m_effects below) runs in every state as a backdrop, menus included.
    if (m_appState == AppState::InMatch || m_appState == AppState::GameOver) {
        drawBattlefieldBackground();

        const BoardView p1View = boardView(0);
        const BoardView p2View = boardView(1);

        const glm::vec2 p1Offset = m_p1PieceVisual.value() - glm::vec2(p1View.activePiece.position());
        const glm::vec2 p2Offset = m_p2PieceVisual.value() - glm::vec2(p2View.activePiece.position());

        drawSingleBoard(0, boardOriginX(0), p1View, p1Offset);
        drawSingleBoard(1, boardOriginX(1), p2View, p2Offset);
        drawInFlightAttacks();
        drawCharacters();
    } else {
        // Menu screens (MainMenu/HostSetup/JoinSetup): the same procedural
        // snowy-mountain backdrop used as drawBattlefieldBackground()'s
        // fallback, so the menu reads as the same winter world rather than
        // a flat clear color.
        const float totalWidth = 2.0f * static_cast<float>(Board::kWidth) + kBoardGap;
        m_renderer.drawWinterLandscape(glm::vec2(-18.0f, -8.0f),
            glm::vec2(totalWidth + 36.0f, Board::kHeight + 18.0f));
    }
    m_effects.draw(m_renderer);

    m_renderer.endFrame();

    renderImGuiFrame();
}

void GameWindow::drawBattlefieldBackground()
{
    const float width = 2.0f * static_cast<float>(Board::kWidth) + kBoardGap;
    if (m_battlefieldTexture != 0) {
        const glm::mat4 inverseView = glm::inverse(m_camera.viewProjectionMatrix());
        const glm::vec2 topLeft(inverseView * glm::vec4(-1, 1, 0, 1));
        const glm::vec2 bottomRight(inverseView * glm::vec4(1, -1, 0, 1));
        const glm::vec2 size = bottomRight - topLeft;
        constexpr float imageAspect = 16.0f / 9.0f;
        const float imageWidth = std::max(size.x, size.y * imageAspect);
        const glm::vec2 uvScale(size.x / imageWidth, size.y / (imageWidth / imageAspect));
        m_renderer.drawQuad(topLeft, size, m_battlefieldTexture, (glm::vec2(1) - uvScale) * 0.5f,
            uvScale, glm::vec4(0.82f, 0.88f, 0.95f, 1.0f));
        // Dark glass behind the gameplay zone keeps every block readable.
        m_renderer.drawQuad(glm::vec2(-2.0f, -2.0f),
            glm::vec2(width + 4.0f, Board::kHeight + 4.0f), glm::vec4(0.01f, 0.035f, 0.07f, 0.18f));
    } else {
        m_renderer.drawWinterLandscape(glm::vec2(-18.0f, -8.0f),
            glm::vec2(width + 36.0f, Board::kHeight + 18.0f));
    }
}

void GameWindow::drawSingleBoard(int playerIndex, float originX, const BoardView& view, glm::vec2 pieceVisualOffset)
{
    drawIceFortress(playerIndex, originX, view);
    m_renderer.drawSoftCircle(glm::vec2(originX - 0.8f, Board::kHeight - 0.10f),
        glm::vec2(Board::kWidth + 1.6f, 1.55f), glm::vec4(0.0f, 0.025f, 0.055f, 0.52f));
    m_renderer.drawIcePanel(glm::vec2(originX, 0.0f), glm::vec2(Board::kWidth, Board::kHeight));
    const SmoothedFloat& settleOffset = playerIndex == 0 ? m_p1StackSettleOffset : m_p2StackSettleOffset;
    const std::vector<RowFlash>& flashes = playerIndex == 0 ? m_p1RowFlashes : m_p2RowFlashes;
    const float rotationPulseRemaining = playerIndex == 0 ? m_p1RotationPulseRemaining : m_p2RotationPulseRemaining;

    // Hairline seams etched into the ice pane replace the old checkerboard cells.
    for (int col = 1; col < Board::kWidth; ++col) {
        m_renderer.drawQuad(glm::vec2(originX + static_cast<float>(col) - 0.012f, 0.0f),
            glm::vec2(0.024f, Board::kHeight), glm::vec4(0.50f, 0.82f, 0.88f, 0.12f));
    }
    for (int row = 1; row < Board::kHeight; ++row) {
        m_renderer.drawQuad(glm::vec2(originX, static_cast<float>(row) - 0.012f),
            glm::vec2(Board::kWidth, 0.024f), glm::vec4(0.50f, 0.82f, 0.88f, 0.10f));
    }

    for (int row = 0; row < Board::kHeight; ++row) {
        for (int col = 0; col < Board::kWidth; ++col) {
            const BlockType cell = view.cellAt(col, row);

            if (cell == BlockType::Empty) {
                continue;
            } else if (!view.gameOver) {
                // A line clear or a garbage insertion has already instantly
                // updated this cell's real (row, col) by the time this
                // draws — settleOffset eases from a just-collapsed/inserted
                // look back to 0, so locked cells visibly slide into their
                // already-correct position instead of teleporting there.
                const glm::vec2 cellPosition(
                    originX + static_cast<float>(col), static_cast<float>(row) + settleOffset.value());
                m_renderer.drawBlock(cellPosition, glm::vec2(1.0f), colorForBlockType(cell));
            }
        }
    }

    if (view.gameOver) {
        drawCollapsedBoardHeap(playerIndex, originX, view);
    }

    for (const RowFlash& flash : flashes) {
        const float alpha = flash.color.a * std::clamp(flash.remaining / flash.duration, 0.0f, 1.0f);
        const glm::vec2 rowPosition(originX, static_cast<float>(flash.row));
        m_renderer.drawQuad(
            rowPosition, glm::vec2(static_cast<float>(Board::kWidth), 1.0f),
            glm::vec4(glm::vec3(flash.color), alpha));
    }

    if (!view.gameOver) {
        const glm::vec4 activeColor = colorForBlockType(view.activePiece.type());

        // A short scale "pop" on every successful rotation — see
        // updatePieceSmoothing() for how it's detected/triggered — so
        // rotating reads as a small physical snap rather than a silent
        // instant swap.
        float pulseScale = 1.0f;
        if (rotationPulseRemaining > 0.0f) {
            const float pulseT = 1.0f - rotationPulseRemaining / kRotationPulseDuration;
            pulseScale = 1.0f + 0.065f * std::sin(glm::pi<float>() * pulseT);
        }
        const glm::vec2 blockSize(pulseScale);
        const glm::vec2 blockCenterAdjust((1.0f - pulseScale) * 0.5f);

        for (const glm::ivec2& cell : view.activePiece.cells()) {
            if (cell.y < 0) {
                continue; // still in the hidden spawn buffer above the board
            }
            const glm::vec2 basePosition(originX + static_cast<float>(cell.x), static_cast<float>(cell.y));
            m_renderer.drawBlock(basePosition + pieceVisualOffset + blockCenterAdjust, blockSize, activeColor);
        }
    }

}

void GameWindow::drawCollapsedBoardHeap(int playerIndex, float originX, const BoardView& view)
{
    const double now = glfwGetTime();
    if (m_boardCollapseStarted[playerIndex] == 0.0) {
        m_boardCollapseStarted[playerIndex] = now;
        m_effects.spawnSnowExplosion(originX + static_cast<float>(Board::kWidth) * 0.5f, 12);
    }
    const float elapsed = static_cast<float>(now - m_boardCollapseStarted[playerIndex]);
    const float boardHeight = static_cast<float>(Board::kHeight);
    int blockIndex = 0;

    for (int row = 0; row < Board::kHeight; ++row) {
        for (int col = 0; col < Board::kWidth; ++col) {
            const BlockType type = view.cellAt(col, row);
            if (type == BlockType::Empty) {
                continue;
            }

            const float seed = static_cast<float>((blockIndex * 67 + row * 17 + col * 31) % 101) / 100.0f;
            const float delay = seed * 0.62f;
            const float age = elapsed - delay;
            const glm::vec2 start(originX + static_cast<float>(col), static_cast<float>(row));

            // The pile has a fixed triangular silhouette; very full boards overlap
            // some fragments instead of growing back into another tall stack.
            int remaining = blockIndex % 108;
            int heapLayer = 0;
            int layerWidth = 20;
            while (remaining >= layerWidth) {
                remaining -= layerWidth;
                ++heapLayer;
                layerWidth = std::max(4, 20 - heapLayer * 2);
            }
            const float tileSize = 0.47f;
            const float layerOffset = (static_cast<float>(Board::kWidth) - static_cast<float>(layerWidth) * tileSize) * 0.5f;
            const glm::vec2 target(
                originX + layerOffset + static_cast<float>(remaining) * tileSize,
                boardHeight - 0.48f - static_cast<float>(heapLayer) * 0.39f + (seed - 0.5f) * 0.08f);

            glm::vec2 position = start;
            float rotation = 0.0f;
            float scale = 1.0f;
            if (age >= 0.0f) {
                const float duration = 1.05f + seed * 0.48f;
                const float t = std::clamp(age / duration, 0.0f, 1.0f);
                const float eased = t * t;
                position = glm::mix(start, target, eased);
                position.x += std::sin(glm::pi<float>() * t) * (seed - 0.5f) * 5.5f;
                position.y -= std::sin(glm::pi<float>() * t) * 0.35f;
                rotation = (seed - 0.5f) * 8.0f * t;
                scale = glm::mix(0.92f, tileSize, eased);

                if (age < 0.18f) {
                    const float flash = 1.0f - age / 0.18f;
                    const float radius = 0.65f + (1.0f - flash) * 0.9f;
                    m_renderer.drawSoftCircle(start - glm::vec2(radius * 0.25f), glm::vec2(radius),
                        glm::vec4(0.64f, 0.94f, 1.0f, flash * 0.55f));
                }
            }

            glm::vec4 color = colorForBlockType(type);
            if (age > 0.7f) {
                const glm::vec3 dirtyIce(0.34f, 0.47f, 0.49f);
                color = glm::vec4(glm::mix(glm::vec3(color), dirtyIce, 0.48f), 1.0f);
            }
            m_renderer.drawBlock(position, glm::vec2(scale), color, rotation);
            ++blockIndex;
        }
    }

    // Broken wall tiles fly in from both sides and cap the settled garbage heap.
    for (int i = 0; i < 20; ++i) {
        const int side = i % 2 == 0 ? -1 : 1;
        const float seed = static_cast<float>((i * 43 + playerIndex * 19) % 97) / 96.0f;
        const float age = elapsed - 0.35f - static_cast<float>(i) * 0.025f;
        if (age < 0.0f) {
            continue;
        }
        const glm::vec2 start(
            side < 0 ? originX - 1.0f : originX + static_cast<float>(Board::kWidth) + 0.5f,
            1.0f + seed * 12.0f);
        const glm::vec2 target(
            originX + 0.25f + seed * (static_cast<float>(Board::kWidth) - 0.8f),
            boardHeight - 0.25f - static_cast<float>(i % 5) * 0.31f);
        const float t = std::clamp(age / (1.15f + seed * 0.35f), 0.0f, 1.0f);
        glm::vec2 position = glm::mix(start, target, t * t);
        position.y -= std::sin(glm::pi<float>() * t) * (0.3f + seed * 0.4f);
        m_renderer.drawBlock(position, glm::vec2(0.52f, 0.24f),
            glm::vec4(0.38f, 0.68f, 0.73f, 1.0f), side * t * (2.4f + seed));
    }
}

void GameWindow::drawIceFortress(int playerIndex, float originX, const BoardView& view)
{
    // The board that actually topped out is the only fortress that fully collapses.
    const bool collapsed = view.gameOver;
    const float damage = m_fortressDamage[playerIndex];
    const float height = static_cast<float>(Board::kHeight);
    const float width = static_cast<float>(Board::kWidth);
    const double now = glfwGetTime();
    int crystalId = 0;

    // Each player's fortress is tinted from their own character's palette
    // (Thomas: icy cyan-blue, Jessica: frosty violet) rather than one
    // shared blue for both, so each side's castle visibly belongs to its
    // player. seed-based jitter (below) still varies individual blocks on
    // top of this base.
    const glm::vec3 brightBase =
        playerIndex == 0 ? glm::vec3(0.58f, 0.86f, 0.91f) : glm::vec3(0.72f, 0.62f, 0.94f);
    const glm::vec3 darkBase =
        playerIndex == 0 ? glm::vec3(0.16f, 0.46f, 0.57f) : glm::vec3(0.30f, 0.20f, 0.50f);

    // Each crystal keeps the existing staged crack, fall, tumble and rubble
    // animation. `heavy` is used for the thick foundation blocks added at
    // the base of each tower (see below) — darker/more desaturated so they
    // read as load-bearing stone rather than more of the same decorative
    // ice, on top of whichever bright/dark tint the caller picked.
    auto crystal = [&](glm::vec2 position, glm::vec2 size, int side, bool bright, float rotation = 0.0f,
                        bool heavy = false) {
        const int id = crystalId++;
        const float seed = static_cast<float>((id * 47 + playerIndex * 13) % 101) / 100.0f;
        const float threshold = 0.12f + seed * 1.18f;
        double& brokenAt = m_fortressBreakTimes[playerIndex][id];
        if (brokenAt == 0.0 && (collapsed || damage > threshold))
            brokenAt = now + (collapsed ? seed * 0.55 : seed * 0.12);
        const float age = brokenAt == 0.0 ? -1.0f : static_cast<float>(now - brokenAt);
        glm::vec3 rgb = bright
            ? brightBase + glm::vec3(seed * 0.12f, seed * 0.05f, seed * 0.03f)
            : darkBase + glm::vec3(seed * 0.10f, seed * 0.11f, seed * 0.10f);
        if (heavy) {
            rgb = glm::mix(rgb, glm::vec3(0.07f, 0.10f, 0.15f), 0.55f);
        }
        const glm::vec4 ice(rgb, 1.0f);

        if (age >= 0.0f) {
            for (int chip = 0; chip < 3; ++chip) {
                const float spread = seed * 0.8f + static_cast<float>(chip) * 0.19f;
                const float floorY = height + kCastleDrop + 0.48f - spread * 0.32f;
                const float rise = 1.1f + spread;
                const float fallTime = (rise + std::sqrt(rise * rise
                    + 28.0f * std::max(0.0f, floorY - position.y))) / 14.0f;
                const float t = std::min(age, fallTime);
                glm::vec2 p = position + glm::vec2(side * (0.4f + spread) * t,
                    -rise * t + 7.0f * t * t);
                p.y = std::min(p.y, floorY);
                m_renderer.drawBlock(p, size * glm::vec2(0.43f, 0.46f), ice,
                    rotation + side * t * (1.8f + spread) + static_cast<float>(chip) * 0.7f);
                const float dustAge = age - fallTime;
                if (dustAge >= 0.0f && dustAge < 0.55f) {
                    const float radius = 0.3f + dustAge * 1.4f;
                    m_renderer.drawSoftCircle(p - glm::vec2(radius * 0.4f, dustAge * 0.4f),
                        glm::vec2(radius, radius * 0.45f),
                        glm::vec4(0.61f, 0.88f, 0.92f, (1.0f - dustAge / 0.55f) * 0.25f));
                }
            }
            return;
        }

        m_renderer.drawBlock(position, size, ice, rotation);
        if (damage > threshold - 0.16f && damage > 0.0f) {
            const glm::vec2 crack = position + size * glm::vec2(0.38f, 0.30f);
            m_renderer.drawBlock(crack, glm::vec2(size.x * 0.40f, 0.032f),
                glm::vec4(0.02f, 0.15f, 0.20f, 1.0f), 0.65f + rotation);
            m_renderer.drawBlock(crack + glm::vec2(size.x * 0.21f, 0.10f),
                glm::vec2(size.x * 0.24f, 0.025f),
                glm::vec4(0.02f, 0.15f, 0.20f, 1.0f), -0.8f + rotation);
        }
    };

    // Staggered masonry and crenellated towers frame the playable ice pane.
    // Rows run the full board height (+1 for overlap with the foundation
    // below) rather than a fixed count, so a taller board never leaves the
    // bottom of the tower without tiles.
    constexpr int kHeavyFoundationRows = 6;
    const int totalRows = Board::kHeight + 1;
    const int normalRows = std::max(1, totalRows - kHeavyFoundationRows);
    const float heavyStartY = kCastleDrop - 0.50f + static_cast<float>(normalRows);
    for (int side : {-1, 1}) {
        const float towerX = side < 0 ? originX - kTowerLeftOffset : originX + width + kTowerRightGap;
        for (int row = 0; row < normalRows; ++row) {
            const float y = kCastleDrop - 0.50f + static_cast<float>(row);
            const float split = row % 2 == 0 ? 0.48f : 0.78f;
            crystal(glm::vec2(towerX, y), glm::vec2(split, 0.97f), side, true);
            crystal(glm::vec2(towerX + split + 0.025f, y),
                glm::vec2(1.28f - split, 0.97f), side, false);
        }

        // Thick, dark foundation along the base of each tower — flared
        // wider than the masonry above (away from the playfield, like a
        // real castle wall widening toward its base) so the castle reads
        // as heavier and more solidly planted. Built from a grid of the
        // same small unit size as the ordinary masonry rows above (not
        // one big slab), each with its own independent crack/fall
        // animation — so damage/collapse breaks it apart stone-by-stone
        // instead of a few oversized chunks flying off at once.
        constexpr int kHeavyColumns = 3;
        const float heavyWidth = kHeavyBaseWidth + kHeavyFlareExtra;
        // The extra width is added entirely on the outward-facing side —
        // for the left tower that's further left (start shifts left,
        // inward/right edge unchanged), for the right tower that's
        // further right (start unchanged, right edge extends out) — so
        // neither tower's flare ever creeps into the playfield. Mirrored
        // exactly by castleOuterEdgeX() above.
        const float heavyX = side < 0 ? (towerX - kTowerRightGap - kHeavyFlareExtra) : (towerX - kTowerRightGap);
        const float columnWidth = heavyWidth / static_cast<float>(kHeavyColumns);
        // Packed almost edge-to-edge (a hairline margin, just enough to
        // keep each block a separately breakable piece) rather than the
        // masonry's usual gap, so the foundation reads as one continuous
        // wall with no daylight showing between stones.
        constexpr float kHeavyTileMargin = 0.01f;
        for (int r = 0; r < kHeavyFoundationRows; ++r) {
            const float y = heavyStartY + static_cast<float>(r);
            for (int c = 0; c < kHeavyColumns; ++c) {
                const float x = heavyX + static_cast<float>(c) * columnWidth;
                crystal(glm::vec2(x, y), glm::vec2(columnWidth - kHeavyTileMargin, 1.0f - kHeavyTileMargin), side,
                    false, 0.0f, true);
            }
        }

        crystal(glm::vec2(towerX - 0.14f, kCastleDrop - 1.0f), glm::vec2(1.58f, 0.47f), side, true);
        for (int i = 0; i < 3; ++i)
            crystal(glm::vec2(towerX - 0.14f + i * 0.57f, kCastleDrop - 1.68f),
                glm::vec2(0.43f, 0.70f), side, true);
    }

    // A solid frozen lintel closes the top of the wall. The bottom no
    // longer gets its own separate row of front-facing foundation tiles —
    // the towers' own heavy foundation (above) plus the glow seam below
    // already read as a solid base without this redundant strip.
    for (int i = 0; i < 12; ++i) {
        const float x = originX + static_cast<float>(i) * width / 12.0f;
        const int side = i < 6 ? -1 : 1;
        const float segmentW = width / 12.0f - 0.025f;
        crystal(glm::vec2(x, kCastleDrop - 0.52f), glm::vec2(segmentW, 0.50f), side, i % 3 == 0);
    }

    if (!collapsed) {
        const float glow = 0.42f * (1.0f - damage * 0.45f);
        // Thin luminous seams unify the separate breakable crystals into one
        // ice barrier, tinted to match this fortress's own player theme.
        const glm::vec3 seamColor = playerIndex == 0 ? glm::vec3(0.25f, 0.92f, 1.0f) : glm::vec3(0.68f, 0.55f, 1.0f);
        // Stops at the top of the heavy foundation rather than running the
        // full board height — past that point the dark foundation blocks
        // (drawn above) already cover this X, so continuing the seam down
        // through them just showed as a stray bright line cutting across
        // the foundation.
        const float seamTop = kCastleDrop - 0.08f;
        const float seamHeight = std::max(0.0f, heavyStartY - seamTop);
        m_renderer.drawQuad(glm::vec2(originX - 0.70f, seamTop),
            glm::vec2(0.10f, seamHeight), glm::vec4(seamColor, glow));
        m_renderer.drawQuad(glm::vec2(originX + width + 0.60f, seamTop),
            glm::vec2(0.10f, seamHeight), glm::vec4(seamColor, glow));
        m_renderer.drawQuad(glm::vec2(originX - 0.02f, kCastleDrop - 0.10f),
            glm::vec2(width + 0.04f, 0.10f), glm::vec4(glm::mix(seamColor, glm::vec3(1.0f), 0.3f), glow));
        m_renderer.drawQuad(glm::vec2(originX - 0.02f, height + kCastleDrop),
            glm::vec2(width + 0.04f, 0.11f), glm::vec4(glm::mix(seamColor, glm::vec3(1.0f), 0.3f), glow));
    }
}


void GameWindow::drawInFlightAttacks()
{
    for (const InFlightAttack& inFlight : inFlightAttacksView()) {
        const int sourceIndex = 1 - inFlight.targetPlayerIndex;
        const float wallX = boardOriginX(inFlight.targetPlayerIndex)
            + (inFlight.targetPlayerIndex == 0 ? static_cast<float>(Board::kWidth) : 0.0f);
        // Launched from the attacking player's own fortress -- specifically
        // its inner tower (the one facing the arena gap) -- rather than
        // from near the character, so the volley visibly comes from the
        // castle itself. Matches drawIceFortress()'s tower geometry: each
        // tower is centered ~0.64 world units past its towerX, and its
        // crenellated turret top sits around y=kCastleDrop-1.3.
        const float sourceOriginX = boardOriginX(sourceIndex);
        const float startX = sourceIndex == 0
            ? sourceOriginX + static_cast<float>(Board::kWidth) + 0.10f + 0.64f
            : sourceOriginX - 1.42f + 0.64f;
        const float startY = kCastleDrop - 1.3f;
        const float impactY = static_cast<float>(Board::kHeight);
        const float targetOriginX = boardOriginX(inFlight.targetPlayerIndex);

        // Allow the windup beat before the volley leaves the tower.
        const float windup = std::min(0.18f, inFlight.durationSeconds * 0.2f);
        if (inFlight.elapsedSeconds < windup) continue;
        const float flightDuration = inFlight.durationSeconds - windup;
        const float t = flightDuration > 0.0f
            ? std::clamp((inFlight.elapsedSeconds - windup) / flightDuration, 0.0f, 1.0f)
            : 1.0f;

        const glm::vec4 color = colorForAttackType(inFlight.attack.type);
        const float bulletLength = bulletLengthForProjectile(inFlight.attack.power);
        const int projectileCount = projectileCountForAttackPower(inFlight.attack.power);

        // A shared rough landing column every sibling in a volley clusters
        // around (each then nudges off it via its own impactColumnOffset),
        // so a volley spreads out across the board rather than every
        // bullet landing on the same spot.
        std::mt19937 sharedRng(seedForProjectile(inFlight.seed, -1));
        std::uniform_real_distribution<float> columnDist(1.0f, static_cast<float>(Board::kWidth - 1));
        const float sharedImpactColumn = columnDist(sharedRng);

        for (int i = 0; i < projectileCount; ++i) {
            std::mt19937 rng(seedForProjectile(inFlight.seed, i));
            const ProjectileFlightParams params = makeFlightParams(rng);

            const float impactX = std::clamp(
                targetOriginX + sharedImpactColumn + params.impactColumnOffset, targetOriginX + 0.5f,
                targetOriginX + static_cast<float>(Board::kWidth) - 0.5f);

            // Reparameterize t per-projectile (see easePower's doc
            // comment) so a volley's bullets don't all move in lockstep —
            // still all arrive together at t=1/localT=1.
            const float localT = std::pow(t, params.easePower);

            // A single cubic Bezier for the whole flight — see
            // cubicBezier()'s doc comment for why this replaces a
            // branching phase-1/phase-2 formula: P1 pulls the early curve
            // up into a launch arc, P2 sits near the wall offset by the
            // "bounce" swerve/lift so the curve visibly deflects there,
            // and it's smooth (no seam) the entire way through. P1's x
            // position varies per-projectile (midpointFraction) rather
            // than always sitting exactly halfway, so a volley's paths
            // fan out instead of running parallel to each other.
            const glm::vec2 p0(startX, startY);
            const glm::vec2 p1(startX + (wallX - startX) * params.midpointFraction, startY - params.arcHeight);
            const glm::vec2 p2(wallX + params.bounceSwerve, startY - params.bounceLift);
            const glm::vec2 p3(impactX, impactY);
            const glm::vec2 pos = cubicBezier(p0, p1, p2, p3, localT);

            // Nose-first along its own direction of travel — see
            // cubicBezierTangent()'s doc comment for why this replaced a
            // free arbitrary spin (it was both wrong for a "missile" look
            // and the actual cause of the reported blinking).
            const glm::vec2 tangent = cubicBezierTangent(p0, p1, p2, p3, localT);
            // glm::rotate(θ) maps local +Y (this shape's un-rotated long
            // axis) to (-sinθ, cosθ) — solving that against the travel
            // direction gives θ = atan2(-tangent.x, tangent.y), not
            // atan2(tangent.x, tangent.y) (verified by hand: a tangent of
            // (1,0) must produce θ=-90° to map +Y onto +X, and only the
            // negated form gives that).
            const float rotation = (tangent.x != 0.0f || tangent.y != 0.0f)
                ? std::atan2(-tangent.x, tangent.y)
                : 0.0f;

            // Elongated nose-to-tail (length along local Y, which is what
            // the rotation above aligns to the travel direction) rather
            // than a plain circle — reads as a directed bullet/missile,
            // not a snowball, while still using the same soft round shader
            // (no hard corners) since the shape is just a stretched circle.
            const float length = bulletLength * params.lengthJitter;
            const float width = length * 0.55f * params.widthJitter;
            const glm::vec2 size(width, length);

            // Curve samples give a continuous wake without emitting particles per frame.
            for (int sample = 7; sample >= 1; --sample) {
                const float tailT = std::max(0.0f, localT - sample * 0.009f);
                const glm::vec2 tail = cubicBezier(p0, p1, p2, p3, tailT);
                const float fade = 1.0f - static_cast<float>(sample) / 8.0f;
                const glm::vec2 tailSize = size * (0.35f + fade * 0.55f);
                m_renderer.drawSoftCircle(tail - tailSize * 0.5f, tailSize,
                    glm::vec4(glm::vec3(color), fade * 0.22f), rotation);
            }
            m_renderer.drawSoftCircle(pos - size * 0.75f, size * 1.5f,
                glm::vec4(glm::vec3(color), 0.14f), rotation);
            m_renderer.drawSoftCircle(pos - size * 0.5f, size, color, rotation);
            const glm::vec2 coreSize = size * 0.55f;
            m_renderer.drawSoftCircle(pos - coreSize * 0.5f, coreSize,
                glm::vec4(0.94f, 0.98f, 1.0f, 0.95f), rotation);
        }
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
        attackMsg.seed = a.seed;
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
                case Protocol::InputActionType::HardDrop:
                    triggerHardDropVisuals(1, boardView(1));
                    p2.hardDrop();
                    break;
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

    // The client always plays Player 2 (see onKey()'s comment), so its own
    // held-key state is exactly what m_p2*KeyDown already tracks — same
    // stuck-glfwGetKey() reasoning as processHeldInput() applies here too.
    Protocol::InputStateMsg msg;
    msg.left = m_p2LeftKeyDown;
    msg.right = m_p2RightKeyDown;
    msg.down = m_p2DownKeyDown;
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
                inFlight.seed = a.seed;
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

namespace
{
// A frosted-ice theme over ImGui's default dark palette: soft cyan
// accents, rounded panels, roomier spacing — applied once at startup so
// every ImGui window (menus, HUD, game-over overlay) shares one winter
// look instead of the stock dark-grey default.
void applyIceTheme()
{
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 12.0f;
    style.ChildRounding = 10.0f;
    style.FrameRounding = 8.0f;
    style.PopupRounding = 10.0f;
    style.ScrollbarRounding = 10.0f;
    style.GrabRounding = 8.0f;
    style.WindowPadding = ImVec2(16.0f, 14.0f);
    style.FramePadding = ImVec2(10.0f, 6.0f);
    style.ItemSpacing = ImVec2(10.0f, 8.0f);
    style.WindowBorderSize = 1.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = ImVec4(0.88f, 0.94f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.60f, 0.70f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.09f, 0.15f, 0.88f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.06f, 0.10f, 0.16f, 0.96f);
    colors[ImGuiCol_Border] = ImVec4(0.40f, 0.70f, 0.92f, 0.45f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.17f, 0.26f, 0.85f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.28f, 0.40f, 0.90f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.36f, 0.50f, 0.95f);
    colors[ImGuiCol_Separator] = ImVec4(0.40f, 0.70f, 0.92f, 0.35f);
    colors[ImGuiCol_Button] = ImVec4(0.16f, 0.32f, 0.46f, 0.88f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.50f, 0.68f, 0.95f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.62f, 0.82f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.45f, 0.80f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.35f, 0.68f, 0.90f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.45f, 0.80f, 1.00f, 1.00f);
}
} // namespace

bool GameWindow::initializeImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImFontConfig fontConfig;
    fontConfig.SizePixels = 18.0f;
    bool loadedFont = false;
#ifdef _WIN32
    if (const char* windowsDirectory = std::getenv("WINDIR")) {
        const auto fontPath = std::filesystem::path(windowsDirectory) / "Fonts" / "segoeuib.ttf";
        if (std::filesystem::exists(fontPath))
            loadedFont = ImGui::GetIO().Fonts->AddFontFromFileTTF(fontPath.string().c_str(), 18.0f) != nullptr;
    }
#endif
    if (!loadedFont) ImGui::GetIO().Fonts->AddFontDefault(&fontConfig);
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    applyIceTheme();

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
    reviewInput();
    ImGui::NewFrame();

    const float width = ImGui::GetIO().DisplaySize.x;
    const float height = ImGui::GetIO().DisplaySize.y;

    if (m_appState == AppState::MainMenu || m_appState == AppState::HostSetup
        || m_appState == AppState::JoinSetup) {
        drawMenuBackground(width, height, m_menuBackground, static_cast<float>(glfwGetTime()));
    }

    switch (m_appState) {
        case AppState::MainMenu:
            handleMenuResult(drawMainMenu(
                width, height, m_menuPortraitBoy, m_menuPortraitGirl, m_menuTitleLogo,
                static_cast<float>(glfwGetTime())));
            break;
        case AppState::HostSetup:
            handleMenuResult(drawHostSetupScreen(width, height, m_network != nullptr, m_networkStatusText));
            break;
        case AppState::JoinSetup:
            handleMenuResult(drawJoinSetupScreen(width, height, m_network != nullptr, m_networkStatusText));
            break;
        case AppState::InMatch:
            drawMatchHud(
                buildHudStats(0), buildHudStats(1), width, height, m_camera.worldToScreenX(castleOuterEdgeX(0), width),
                m_camera.worldToScreenX(castleOuterEdgeX(1), width));
            break;
        case AppState::GameOver: {
            drawMatchHud(
                buildHudStats(0), buildHudStats(1), width, height, m_camera.worldToScreenX(castleOuterEdgeX(0), width),
                m_camera.worldToScreenX(castleOuterEdgeX(1), width));
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
    captureReview(m_window, m_width, m_height);
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

        if (m_gameOverWinnerIndex == -1) {
            // Simultaneous overflow (a draw): both react the same way.
            m_characters[0].onLose();
            m_characters[1].onLose();
        } else {
            m_characters[m_gameOverWinnerIndex].onWin();
            m_characters[1 - m_gameOverWinnerIndex].onLose();
        }
    }
}

void GameWindow::updateCharacters(float deltaTime)
{
    m_characterAsset->update(deltaTime);
    for (CharacterController& character : m_characters) {
        character.update(deltaTime);
    }


    if (m_appState != AppState::InMatch || m_networkConfig.role == NetworkRole::Client) {
        // Near-defeat/frozen both need direct GameManager access, which a
        // network client never has (its boards are display-only mirrors —
        // see BoardView's own doc comment). Attack success/received and
        // win/lose still work for a client, since those are triggered from
        // onAttackLanded()/checkForGameOver(), which the client's packet
        // handler calls directly with host-sent data.
        return;
    }

    constexpr int kNearDefeatRowThreshold = Board::kHeight / 4;
    for (int i = 0; i < 2; ++i) {
        GameManager& gm = m_match.player(i).gameManager();
        if (gm.isGameOver()) {
            continue;
        }

        const bool nearDefeat = gm.board().highestOccupiedRow() <= kNearDefeatRowThreshold;
        if (nearDefeat && !m_nearDefeatTriggered[i]) {
            m_characters[i].onNearDefeat();
        }
        m_nearDefeatTriggered[i] = nearDefeat;
    }
}

void GameWindow::drawCharacters()
{
    // Both characters stand in the gap between the two boards, facing
    // each other like a versus battle, rather than each floating above
    // its own board. kInterCharacterGap is the breathing room between
    // their facing edges; everything else follows from the board gap and
    // the characters' own footprint.
    constexpr float kInterCharacterGap = 3.2f;
    const float renderedSize = kCharacterPlaceholderSize * kCharacterRenderScale;
    const float gapCenterX = static_cast<float>(Board::kWidth) + kBoardGap / 2.0f;
    const float halfSpacing = kInterCharacterGap / 2.0f + renderedSize / 2.0f;

    // Bottom edge flush with the boards' bottom row, so they read as
    // standing on the arena floor rather than floating. Nudged up slightly
    // from that flush position so their feet don't crowd the very bottom
    // edge of the frame.
    constexpr float kStandingLift = -0.6f;
    const float topY = static_cast<float>(Board::kHeight) - renderedSize * 1.35f - kStandingLift;

    const float centerX[2] = {gapCenterX - halfSpacing, gapCenterX + halfSpacing};
    for (int i = 0; i < 2; ++i) {
        const glm::vec2 topLeft(centerX[i] - renderedSize / 2.0f, topY);
        m_renderer.drawSoftCircle(glm::vec2(centerX[i]-1.8f, Board::kHeight-0.45f),
            glm::vec2(3.6f, 0.6f), glm::vec4(0.015f, 0.04f, 0.055f, 0.55f));
        m_characterAsset->draw(m_renderer, topLeft, m_characters[i].emotion(), i, m_characters[i].animationSeconds());
    }
}

void GameWindow::resetPieceSmoothingState()
{
    m_p1LastPieceGeneration = -1;
    m_p2LastPieceGeneration = -1;
    m_remoteView[0] = BoardView{};
    m_remoteView[1] = BoardView{};
    m_remoteInFlightAttacks.clear();

    m_p1LastRotationState = 0;
    m_p2LastRotationState = 0;
    m_p1RotationPulseRemaining = 0.0f;
    m_p2RotationPulseRemaining = 0.0f;
    m_p1StackSettleOffset.snapTo(0.0f);
    m_p2StackSettleOffset.snapTo(0.0f);
    m_p1RowFlashes.clear();
    m_p2RowFlashes.clear();

    // Called from every match-(re)start/reset path, so it doubles as the
    // reset point for Phase 6's per-player character reaction state too.
    m_characters[0].reset();
    m_characters[1].reset();
    m_nearDefeatTriggered[0] = false;
    m_nearDefeatTriggered[1] = false;
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

void GameWindow::windowFocusCallback(GLFWwindow* window, int focused)
{
    auto* self = static_cast<GameWindow*>(glfwGetWindowUserPointer(window));
    if (self != nullptr) {
        self->onFocusChanged(focused == GLFW_TRUE);
    }
}

void GameWindow::keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
{
    auto* self = static_cast<GameWindow*>(glfwGetWindowUserPointer(window));
    if (self != nullptr) {
        self->onKey(key, action);
    }
}
