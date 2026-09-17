#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "Engine/AnimationSystem.h"
#include "Engine/Camera.h"
#include "Engine/OpenGLLoader.h"
#include "Engine/ParticleSystem.h"
#include "Engine/Renderer.h"
#include "Game/Board.h"
#include "Game/Match.h"
#include "Game/Tetromino.h"
#include "Network/Protocol.h"

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
};

// Owns the GLFW window/OpenGL context, drives the game loop, and renders
// the current match state: both players' boards, in-flight snow attacks
// with a particle trail, ambient snowfall, and clear/impact particle
// bursts with camera shake.
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
    // Everything the rendering layer needs to draw one player's board,
    // regardless of where it came from: read straight off a live
    // GameManager for a locally/host-simulated player, or rebuilt from
    // network messages for a network client's remote view of either
    // player. Carries no color/visual info of its own.
    struct BoardView
    {
        std::array<std::array<BlockType, Board::kWidth>, Board::kHeight> cells{};
        Tetromino activePiece{BlockType::I, glm::ivec2(0, -100)};
        int activePieceGeneration = -1;
        bool gameOver = false;

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
    void processHeldInput(float deltaTime);
    void pollHeldKey(
        HeldKeyState& state, bool isDown, float deltaTime, float repeatInterval, GameManager& target,
        void (GameManager::*action)());
    void requestReset();

    // Eases each player's active-piece render position toward its logical
    // grid position every frame (see Engine/AnimationSystem), snapping
    // instead whenever a genuinely new piece has spawned.
    void updatePieceSmoothing(float deltaTime);
    void updateAmbientSnow(float deltaTime);

    BoardView boardView(int playerIndex) const;
    const std::vector<InFlightAttack>& inFlightAttacksView() const;

    // Effect hooks, wired to GameManager/Match callbacks in initialize().
    void onLinesCleared(int playerIndex, const std::vector<Board::ClearedLine>& clearedLines);
    void onAttackLanded(int targetPlayerIndex, const SnowAttack& attack);

    void render();
    void drawSingleBoard(float originX, const BoardView& view, glm::vec2 pieceVisualOffset);
    void drawInFlightAttacks();

    // --- Milestone 6: LAN link -------------------------------------------
    bool initializeNetwork();
    void pollNetwork();
    void hostBroadcastLiveState();
    void hostSendBoardSnapshot(int playerIndex);
    void hostHandleClientPacket(const std::vector<uint8_t>& bytes);
    void clientSendInputState();
    void clientSendInputAction(Protocol::InputActionType action);
    void clientHandleHostPacket(const std::vector<uint8_t>& bytes);

    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    GLFWwindow* m_window = nullptr;
    int m_width;
    int m_height;
    const char* m_title;

    NetworkConfig m_networkConfig;
    std::unique_ptr<NetworkSession> m_network;

    Camera m_camera;
    Renderer m_renderer;
    Match m_match; // Local/Host: the real simulation. Client: unused.
    ParticleSystem m_particles;

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

    float m_ambientSnowTimer = 0.0f;
    std::mt19937 m_ambientRng{std::random_device{}()};
};
