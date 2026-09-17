# Tetrisnow

A competitive 2D snow-battle game built on Tetris mechanics. Two players
face off over LAN — clearing lines converts blocks into snow energy, which
becomes snow bombs launched at the opponent's board.

## Status

Milestone 2: OpenGL Rendering Engine — a Renderer/Camera/ShaderManager/
TextureManager draw a placeholder board grid, solid-color test blocks, and a
textured quad through a real OpenGL 3.3 core pipeline. No gameplay yet.

## Stack

Pure C++ and OpenGL — no application framework:

- **GLFW** — window creation, OpenGL context, input
- A small hand-rolled OpenGL function loader (`Engine/OpenGLLoader`) instead
  of GLEW/GLAD — this project only calls ~25 GL 3.3 functions, so declaring
  and resolving exactly those keeps the dependency list to just GLFW
- **GLM** — vector/matrix math
- Networking (Milestone 6) will use raw sockets or ENet — no Qt Network

## Dependencies

- **CMake** 3.21+
- A C++20 compiler (MSVC / Visual Studio 2022, or MinGW-w64)
- System OpenGL (ships with the GPU driver / Windows — nothing to install)

GLFW and GLM are fetched automatically by CMake on first configure
(`FetchContent`); this needs network access once and is then cached. Both
are small and build in well under a minute — there's no Qt-style SDK
install or multi-hour build involved.

## Building (Windows)

From a **Developer PowerShell/Command Prompt for VS 2022** (or after running
`vcvars64.bat`, so `cl.exe` is on PATH):

```
cmake -S . -B build -G Ninja
cmake --build build
```

(Any CMake generator works — Ninja is just fast. `-G "Visual Studio 17 2022" -A x64` works too, without needing a Developer shell.)

Run the produced `Tetrisnow.exe` from the `build` directory.

## Project Layout

```
Tetrisnow/
├── Engine/    Rendering, OpenGL, particles, animation
├── Game/      Board, tetrominoes, snow attacks, scoring (Milestone 3+)
├── Network/   LAN discovery, client/server (Milestone 6)
├── UI/        Menus, lobby, HUD (Milestone 7, likely Dear ImGui)
└── Assets/    Textures, shaders, sounds
```
