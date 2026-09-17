#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "Engine/AnimationSystem.h"
#include "Engine/Camera.h"
#include "Engine/CharacterAsset.h"
#include "Engine/CharacterRenderer.h"
#include "Engine/EffectManager.h"
#include "Engine/OpenGLLoader.h"
#include "Engine/Renderer.h"
#include "Engine/TextureManager.h"
#include "FaceAvatar/FaceAvatarSystem.h"
#include "FaceAvatar/FaceExpressionController.h"
#include "Game/Board.h"
#include "Game/CharacterController.h"
#include "Game/Match.h"
#include "Game/Tetromino.h"
#include "Network/Protocol.h"
#include "UI/Hud.h"
#include "UI/MenuScreens.h"

struct GLFWwindow;
class NetworkSession;

// How this process participates in a match. Local is the original
// same-window two-player mode; Host/Client are Milestone 6's LAN link —
// the host runs the one authoritative Match (exactly like Local's, just
// with player 1's input arriving over the network instead of from a
// second local keyset), and the client is a thin display + input sender
// that never simulates gameplay itself.
enum class NetworkRole
{
    Local,
    Host,
    Client,
};

struct NetworkConfig
{
    NetworkRole role = NetworkRole::Local;
    std::string hostAddress; // Client only: the host's address to connect to.
    uint16_t port = 7777;
    // True when role/hostAddress/port came from CLI args (--local/--host/
    // --join) and the app should skip straight past the main menu.
    bool skipMenu = false;
};

// Which screen is currently showing. Milestone 7 added everything except
// InMatch, which is all that used to exist.
enum class AppState
{
    MainMenu,
    HostSetup,
    JoinSetup,
    InMatch,
    GameOver,
};

// Owns the GLFW window/OpenGL context, drives the game loop, and renders
// the current app state: menus, the in-match view (both players' boards,
// in-flight snow attacks, ambient snowfall, particle effects) with its
// HUD overlay, and the game-over screen. Also owns all Milestone 6/7
// orchestration: network host/client setup and the ImGui menu/HUD layer.
//
// With Qt gone, there's no separate OS-level "main window" hosting a
// widget — GameWindow both is the window and runs the loop, which is all
// the structure a single-window desktop game needs.
class GameWindow
{
public:
    GameWindow(int width, int height, const char* title, NetworkConfig networkConfig = {});
    ~GameWindow();

    GameWindow(const GameWindow&) = delete;
    GameWindow& operator=(const GameWindow&) = delete;

    // Creates the window/context and loads OpenGL functions. Returns false
    // on failure (details are printed to stderr).
    bool initialize();

    // Runs the main loop until the window is closed.
    void run();

private:
    // Everything the rendering/HUD layer needs for one player, regardless
    // of where it came from: read straight off a live GameManager for a
    // locally/host-simulated player, or rebuilt from network messages for
    // a network client's view of either player. Carries no color/visual
    // info of its own.
    struct BoardView
    {
        std::array<std::array<BlockType, Board::kWidth>, Board::kHeight> cells{};
        Tetromino activePiece{BlockType::I, glm::ivec2(0, -100)};
        int activePieceGeneration = -1;
        bool gameOver = false;
        BlockType nextPieceType = BlockType::Empty;
        int score = 0;
        int snowEnergy = 0;

        BlockType cellAt(int col, int row) const
        {
            if (col < 0 || col >= Board::kWidth || row < 0 || row >= Board::kHeight) {
                return BlockType::Empty;
            }
            return cells[static_cast<size_t>(row)][static_cast<size_t>(col)];
        }
    };

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
    void onFocusChanged(bool focused);
    void processHeldInput(float deltaTime);
    void pollHeldKey(
        HeldKeyState& state, bool isDown, float deltaTime, float repeatInterval, GameManager& target,
        void (GameManager::*action)());
    void requestReset();

    // Eases each player's active-piece render position toward its logical
    // grid position every frame (see Engine/AnimationSystem), snapping
    // instead whenever a genuinely new piece has spawned.
    void updatePieceSmoothing(float deltaTime);

    // Advances each player's CharacterController and drives its
    // event-triggered emotions (near-defeat, frozen) from live
    // GameManager state. Attack success/received and win/lose are
    // triggered directly from onAttackLanded()/checkForGameOver() instead,
    // right where those events are already detected.
    void updateCharacters(float deltaTime);
    void drawCharacters();

    BoardView boardView(int playerIndex);
    const std::vector<InFlightAttack>& inFlightAttacksView() const;

    // Effect hooks, wired to GameManager/Match callbacks in initialize().
    void onLinesCleared(int playerIndex, const std::vector<Board::ClearedLine>& clearedLines);
    void onAttackLanded(int targetPlayerIndex, const SnowAttack& attack);

    void render();
    void drawSingleBoard(float originX, const BoardView& view, glm::vec2 pieceVisualOffset);
    void drawInFlightAttacks();

    // --- Milestone 6: LAN link -------------------------------------------
    bool initializeNetwork();
    void wireNetworkCallbacks();
    void pollNetwork();
    void hostBroadcastLiveState();
    void hostSendBoardSnapshot(int playerIndex);
    void hostHandleClientPacket(const std::vector<uint8_t>& bytes);
    void clientSendInputState();
    void clientSendInputAction(Protocol::InputActionType action);
    void clientHandleHostPacket(const std::vector<uint8_t>& bytes);

    // --- Milestone 7: menus, HUD, app state -------------------------------
    bool initializeImGui();
    void shutdownImGui();
    void renderImGuiFrame();
    void handleMenuResult(const MenuResult& result);
    void startLocalMatch();
    void startHosting(uint16_t port);
    void startJoining(const std::string& hostAddress, uint16_t port);
    void returnToMainMenu();
    void updateAppState();
    void checkForGameOver();
    void resetPieceSmoothingState();
    HudPlayerStats buildHudStats(int playerIndex);

    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void windowFocusCallback(GLFWwindow* window, int focused);

    GLFWwindow* m_window = nullptr;
    int m_width;
    int m_height;
    const char* m_title;
    bool m_imguiInitialized = false;

    AppState m_appState = AppState::MainMenu;
    int m_gameOverWinnerIndex = -1;

    NetworkConfig m_networkConfig;
    std::unique_ptr<NetworkSession> m_network;
    std::string m_networkStatusText;

    Camera m_camera;
    Renderer m_renderer;
    Match m_match; // Local/Host: the real simulation. Client: unused.
    EffectManager m_effects;
    TextureManager m_textureManager;
    // Constructed in initialize() once the GL context is current, loading
    // sprite art from Assets/Characters/ — see Engine/SpriteCharacterAsset.
    std::unique_ptr<CharacterAsset> m_characterAsset;
    // Independent overlay showing the local player's uploaded face photo
    // (Phase 1 of the Face Avatar System — see FaceAvatar/FaceAvatarSystem.h).
    // Unrelated to m_characterAsset's per-emotion sprite sheets.
    FaceAvatarSystem m_faceAvatar;
    // Phase 3: drives m_faceAvatar's deformation/shader look from a
    // FaceEmotion. Stays Neutral until Phase 4 wires real gameplay events.
    FaceExpressionController m_faceExpression;

    HeldKeyState m_p1Left;
    HeldKeyState m_p1Right;
    HeldKeyState m_p1Down;
    HeldKeyState m_p2Left;
    HeldKeyState m_p2Right;
    HeldKeyState m_p2Down;

    // Host only: player 1's most recently received held-key state,
    // consumed by processHeldInput() exactly like a local glfwGetKey()
    // read would be.
    Protocol::InputStateMsg m_remoteInput;

    // Client only: the two boards as last described by the host, plus a
    // matching display-only in-flight-attack list.
    BoardView m_remoteView[2];
    std::vector<InFlightAttack> m_remoteInFlightAttacks;

    SmoothedVec2 m_p1PieceVisual;
    SmoothedVec2 m_p2PieceVisual;
    int m_p1LastPieceGeneration = -1;
    int m_p2LastPieceGeneration = -1;

    // Per-player comic-reaction state (Phase 6) — see Game/CharacterController.h.
    // m_nearDefeatTriggered is a rising-edge flag so onNearDefeat() fires
    // once per crossing, not every frame the board stays tall.
    CharacterController m_characters[2];
    float m_characterAnimationSeconds = 0.0f;
    bool m_nearDefeatTriggered[2] = {false, false};
};
