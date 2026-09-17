# Tetrisnow

A competitive 2D snow-battle game built on Tetris mechanics. Two players
face off over LAN — clearing lines converts blocks into snow energy, which
becomes snow attacks launched at the opponent's board.

## Status

Milestones 1–6 done: OpenGL rendering, core Tetris gameplay, snow-battle
attacks, animation/particle effects, and LAN multiplayer. Milestone 7 (UI
and polish) is next.

## Stack

Pure C++ and OpenGL — no application framework:

- **GLFW** — window creation, OpenGL context, input
- A small hand-rolled OpenGL function loader (`Engine/OpenGLLoader`) instead
  of GLEW/GLAD — this project only calls ~25 GL 3.3 functions, so declaring
  and resolving exactly those keeps the dependency list to just GLFW
- **GLM** — vector/matrix math
- **ENet** — reliable/unreliable UDP for the LAN link (Milestone 6)

## Dependencies

- **CMake** 3.21+
- A C++20 compiler (MSVC / Visual Studio 2022, or MinGW-w64)
- System OpenGL (ships with the GPU driver / Windows — nothing to install)

GLFW, GLM, and ENet are fetched automatically by CMake on first configure
(`FetchContent`); this needs network access once and is then cached. All are
small and build in well under a minute — there's no Qt-style SDK install or
multi-hour build involved.

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
Tetrisnow.exe                     Local two-player (same window/keyboard)
Tetrisnow.exe --host [port]       Host a LAN match (default port 7777)
Tetrisnow.exe --join <ip> [port]  Join a host at <ip>[:port]
```

In a hosted match, the host plays Player 1 (arrows) and the client plays
"Player 2" using the same arrow-key layout on their own machine — the host
simulates the whole match and streams state to the client. `R` resets the
match from either side.

### Controls

**Local two-player** (one window, one keyboard):

- Player 1: ← → move, ↓ soft drop, ↑ rotate CW, Enter hard drop
- Player 2: A/D move, S soft drop, W rotate CW, Left Ctrl hard drop
- `R`: reset the match (both boards)

**Hosted/joined match**: both sides use ← → ↓ move/soft-drop, ↑ rotate CW,
Enter hard drop, `R` reset — each machine controls its own player.

## Project Layout

```
Tetrisnow/
├── Engine/    Rendering, OpenGL, particles, animation, the game loop
├── Game/      Board, tetrominoes, snow attacks, scoring — no rendering
│              or networking dependencies
├── Network/   ENet transport + wire protocol for the LAN link
├── UI/        Menus, lobby, HUD (Milestone 7, likely Dear ImGui)
└── Assets/    Textures, shaders, sounds
```

See `HANDOFF.md` for the full design/architecture notes and how to resume
development from a fresh session.
