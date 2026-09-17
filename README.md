# Tetrisnow

A competitive 2D snow-battle game built on Tetris mechanics. Two players
face off over LAN — clearing lines converts blocks into snow energy, which
becomes snow attacks launched at the opponent's board.

## Status

Milestones 1–7 done: OpenGL rendering, core Tetris gameplay, snow-battle
attacks, animation/particle effects, LAN multiplayer, and a menu/HUD UI.
This completes the original project plan.

## Stack

Pure C++ and OpenGL — no application framework:

- **GLFW** — window creation, OpenGL context, input
- A small hand-rolled OpenGL function loader (`Engine/OpenGLLoader`) instead
  of GLEW/GLAD — declares and resolves exactly the ~35 GL functions this
  project (plus Dear ImGui's OpenGL3 backend) actually calls
- **GLM** — vector/matrix math
- **ENet** — reliable/unreliable UDP for the LAN link (Milestone 6)
- **Dear ImGui** — menus and the in-match HUD (Milestone 7)

## Dependencies

- **CMake** 3.21+
- A C++20 compiler (MSVC / Visual Studio 2022, or MinGW-w64)
- System OpenGL (ships with the GPU driver / Windows — nothing to install)

GLFW, GLM, ENet, and Dear ImGui are fetched automatically by CMake on first
configure (`FetchContent`); this needs network access once and is then
cached. All are small and build in well under a minute — there's no
Qt-style SDK install or multi-hour build involved.

## Building (Windows)

From a **Developer PowerShell/Command Prompt for VS 2022** (or after running
`vcvars64.bat`, so `cl.exe` is on PATH):

```
cmake -S . -B build -G Ninja
cmake --build build
```

(Any CMake generator works — Ninja is just fast. `-G "Visual Studio 17 2022" -A x64` works too, without needing a Developer shell.)

Run the produced `Tetrisnow.exe` from the `build` directory.

## Running

```
Tetrisnow.exe                     Show the main menu
Tetrisnow.exe --local             Local two-player (same window/keyboard)
Tetrisnow.exe --host [port]       Host a LAN match (default port 7777)
Tetrisnow.exe --join <ip> [port]  Join a host at <ip>[:port]
```

With no arguments, the main menu lets you pick Local / Host / Join
interactively — the CLI flags are shortcuts that skip straight past it. In
a hosted match, the host plays Player 1 (WASD) and the client plays
Player 2 (arrows) on their own machine — the host simulates the whole
match and streams state to the client. `R` resets the match (or starts a
rematch from the game-over screen) from either side.

### Controls

**Local two-player** (one window, one keyboard):

- Player 1: A/D move, S soft drop, W rotate CW, Left Ctrl hard drop
- Player 2: ← → move, ↓ soft drop, ↑ rotate CW, Right Ctrl hard drop
- `R`: reset the match (both boards)

**Hosted/joined match**: the host uses Player 1's keyset (WASD + Left
Ctrl), the client uses Player 2's keyset (arrows + Right Ctrl) on its own
machine; `R` resets/rematches from either side.

## Project Layout

```
Tetrisnow/
├── Engine/    Rendering, OpenGL, particles, animation, the game loop,
│              network orchestration, and the app-state machine
├── Game/      Board, tetrominoes, snow attacks, scoring — no rendering,
│              networking, or UI dependencies
├── Network/   ENet transport + wire protocol for the LAN link
├── UI/        Dear ImGui menu screens and in-match HUD
└── Assets/    Textures, shaders, sounds
```

See `HANDOFF.md` for the full design/architecture notes — including how
Dear ImGui's OpenGL3 backend was made to share `Engine/OpenGLLoader`
instead of bundling its own (a non-obvious fix, worth reading before
touching that integration) — and how to resume development from a fresh
session.
