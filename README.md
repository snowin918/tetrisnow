# Tetrisnow

A competitive 2D snow-battle game built on Tetris mechanics. Two players
face off over LAN — clearing lines converts blocks into snow energy, which
becomes snow bombs launched at the opponent's board.

## Status

Milestone 1: Project Foundation — Qt6 application shell with an OpenGL
rendering surface and a fixed-rate game loop. No gameplay yet.

## Dependencies

- **CMake** 3.21+
- **Qt6** (Widgets, OpenGLWidgets modules) — Qt 6.5 or newer recommended
- A C++20 compiler:
  - **MSVC** (Visual Studio 2022 Build Tools), or
  - **MinGW-w64**, matching whichever Qt6 kit you install

Qt6 does not need to be on your system PATH; you point CMake at it via
`CMAKE_PREFIX_PATH` (see below).

## Building (Windows)

1. Install Qt6 via the [Qt Online Installer](https://www.qt.io/download-qt-installer)
   (select the Widgets/OpenGL components for your chosen compiler kit, e.g.
   `msvc2022_64` or `mingw_64`) and CMake.
2. Configure, pointing CMake at your Qt6 install:

   ```
   cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.7.0/msvc2022_64"
   ```

3. Build:

   ```
   cmake --build build --config Debug
   ```

4. Run the produced `Tetrisnow.exe` from the `build/Debug` (MSVC) or
   `build` (MinGW) directory.

## Project Layout

```
Tetrisnow/
├── Engine/    Rendering, OpenGL, particles, animation
├── Game/      Board, tetrominoes, snow attacks, scoring (Milestone 3+)
├── Network/   LAN discovery, client/server (Milestone 6)
├── UI/        Menus, lobby, HUD
└── Assets/    Textures, shaders, sounds
```
