# Tetrisnow — Handoff / Continuation Notes

This file exists so a **new chat session** (with no memory of this one) can
pick up development where it left off. Paste something like *"Read
HANDOFF.md and continue with Milestone 6"* to resume.

## What this project is

A competitive 2D snow-battle game built on Tetris mechanics, for two
players over LAN (per the original brief). Clearing lines converts blocks
into snow energy → a snow bomb → an attack launched at the opponent's
board (garbage rows), themed as a snowball fight.

## Status: Milestones 1–6 done, building toward 7

| # | Milestone | Status |
|---|---|---|
| 1 | Project Foundation | ✅ Done |
| 2 | OpenGL Rendering Engine | ✅ Done |
| 3 | Tetris Core Gameplay | ✅ Done |
| 4 | Snow Battle Mechanics | ✅ Done |
| 5 | Animation and Effects | ✅ Done |
| 6 | LAN Multiplayer | ✅ Done |
| 7 | UI and Polish | ⬜ Not started |

The original prompt's full milestone-by-milestone plan (including the
"stop after each milestone and wait for approval" workflow) still applies
— the user has been approving each milestone before moving to the next.

Git history (`main` branch, one commit per milestone plus a couple of
targeted fixes) — check `git log` for the current head, this list may lag:

```
<Milestone 6 commit — see git log>
b62c894 Add HANDOFF.md for resuming work in a new session
1e247b1 Milestone 5: animation and effects
b07c7a1 Fix unreliable held-key movement/soft-drop
aff7a43 Milestone 4: snow battle mechanics
a4354c7 Reverse tetromino rotation direction
f0d0f0e Milestone 3: Tetris core gameplay
b58a040 Replace Qt6 with GLFW + hand-rolled OpenGL loader
7deb1cf Milestone 2: OpenGL rendering engine
f570600 Milestone 1: project foundation (Qt6 + OpenGL app shell)
```

Local commits are **ahead of `origin/main`** (never pushed) — ask the user
before pushing.

## Important pivot: Qt6 → GLFW

The project *started* on Qt6 per the original brief, but Qt6 proved far
too heavy to build in this environment (a from-source vcpkg build ran 45+
minutes without finishing). **The user explicitly said to drop Qt6** and
use plain C++ + OpenGL only. The stack since Milestone 2 (redone) is:

- **GLFW** — window/input/GL context (fetched via CMake `FetchContent`)
- **GLM** — math (fetched via `FetchContent`)
- **A hand-rolled OpenGL function loader** (`Engine/OpenGLLoader`) instead
  of GLEW/GLAD — only ~25 GL 3.3 functions are used, declared/resolved
  manually via `glfwGetProcAddress`
- No image library yet (textures are procedural so far); `stb_image.h`
  would be the natural choice if/when real sprite art is added

**Do not suggest reintroducing Qt.** If networking needs a GUI toolkit
later, stay within the "pure C++/OpenGL, minimal deps" philosophy (e.g.
Dear ImGui pairs well with GLFW+OpenGL for Milestone 7's menus/HUD — not
yet decided, just the most likely candidate).

## Build environment specifics (this machine)

No Qt, no system-wide CMake on PATH, but **Visual Studio 2022 Community**
is installed with its own bundled CMake/Ninja. Known-good paths used this
session:

```
$vcvars = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
$cmake  = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
```

Configure + build (from PowerShell, via `cmd /c` to source vcvars first):

```powershell
cmd /c "`"$vcvars`" >nul && `"$cmake`" -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug && `"$cmake`" --build build"
```

First configure fetches GLFW+GLM from GitHub (needs network once, then
cached in `build/_deps`). Configure+build together take well under a
minute once deps are cached. Run `build\Tetrisnow.exe`.

A previous attempt used **vcpkg** to fetch a full Qt6 SDK — that whole
approach (and a ~1.85GB scratch `vcpkg` clone in the user's home dir) was
abandoned and cleaned up. Don't resurrect it.

## Testing approach used so far

There's no GUI test framework. Verification has been a mix of:

1. **Standalone logic tests** — small throwaway `.cpp` files compiled ad
   hoc with `cl.exe` directly against the relevant `Game/*.cpp` sources
   (not part of the CMake project), exercising `Board`, `GameManager`,
   `SnowAttack` logic with `assert()`. Written to the scratchpad dir, not
   committed. This is the most reliable way to verify gameplay logic.
2. **Live visual checks** — launching `Tetrisnow.exe` via PowerShell
   `Start-Process`, then using **`PostMessage` directly to the window's
   HWND** (not `SendKeys`/`SetForegroundWindow`) to simulate key input,
   and taking screenshots to inspect the result.

   **Important lesson learned:** `SendKeys` + `AppActivate`/
   `SetForegroundWindow` proved unreliable — `AppActivate("Tetrisnow")`
   once matched VS Code's window instead (its title also contains
   "Tetrisnow"), nearly causing a stray keystroke to land in the wrong
   place. **Prefer `PostMessage` with a real `WM_KEYDOWN`/`WM_KEYUP`
   posted straight to the game's `MainWindowHandle`** — it works
   regardless of OS focus and can't leak into other windows. Compute the
   scancode via `MapVirtualKey` and set the extended-key bit (bit 24) for
   arrow keys. See any of the milestone conversations for working
   PowerShell snippets.
3. Forcing a genuine full-row line clear via synthetic input turned out to
   be impractical (needs precise per-piece column placement) — this was
   tried and abandoned as low value for the effort. If you need to verify
   line-clear-triggered behavior (particles, attacks), prefer driving
   `Board`/`GameManager` directly in a standalone test rather than trying
   to script real gameplay.

## Known gaps / things not yet live-verified

- The actual line-clear explosion particles + camera shake have **not**
  been visually observed live (only unit-tested at the `Board` level and
  verified by code review) — see point above for why.
- No automated test suite is part of the CMake build; all verification so
  far is ad hoc.

## Architecture as built

```
Tetrisnow/
├── CMakeLists.txt        # FetchContent for GLFW+GLM+ENet, no Qt/vcpkg
├── main.cpp               # entry point: parses --host/--join into a
│                           #   NetworkConfig, constructs GameWindow, runs()
├── Engine/
│   ├── OpenGLLoader.*      # hand-rolled GL 3.3 function loader
│   ├── Camera.*            # 2D ortho camera, world Y down, + screen shake
│   ├── Renderer.*          # single unit-quad + shader, drawQuad() API
│   ├── ShaderManager.*     # compiles/caches GLSL programs
│   ├── TextureManager.*    # texture cache (currently only used for a
│   │                       #   Milestone-2 test checkerboard; unused by
│   │                       #   current gameplay — kept for future sprites)
│   ├── ParticleSystem.*    # CPU particles, drawn via Renderer::drawQuad
│   ├── AnimationSystem.*   # SmoothedVec2 — frame-rate-independent easing
│   └── GameWindow.*        # owns GLFW window/loop, input routing,
│                           #   rendering of both boards + effects, AND
│                           #   (Milestone 6) all host/client network
│                           #   orchestration — see "LAN multiplayer" below
├── Game/                   # zero OpenGL/GLFW/network dependencies — pure
│   │                       #   logic, unchanged in spirit since Milestone 3
│   ├── BlockType.h         # enum I/O/T/S/Z/J/L/Snow/Empty (no color!)
│   ├── Board.*             # 10x20 grid, collision, clearFullLines(),
│   │                       #   addGarbageRows() (opponent damage model)
│   ├── Tetromino.*         # shape/rotation data, pure geometry
│   ├── ScoreSystem.*       # classic line-clear point table
│   ├── GameManager.*       # one board's full Tetris session: gravity,
│   │                       #   move/rotate/drop, locking, 7-bag RNG,
│   │                       #   callbacks (onLinesCleared, onPieceLocked,
│   │                       #   onGameOver) — onPieceLocked added in
│   │                       #   Milestone 6 so the host knows a board's
│   │                       #   grid changed even when nothing cleared
│   ├── SnowAttack.*        # attack tier (Snowball/SnowBomb/Avalanche)
│   │                       #   from lines-cleared count
│   ├── Player.*            # wraps GameManager + name + snowEnergy stat
│   └── Match.*             # coordinates 2 Players, in-flight attacks,
│                           #   onAttackLanded callback — knows nothing
│                           #   about whether either Player is local or
│                           #   driven over the network (Milestone 6 never
│                           #   had to touch this file)
├── Network/                # Milestone 6 — transport + wire format only,
│   │                       #   no Game/ or Engine/ dependency
│   ├── NetworkSession.*    # thin ENet wrapper: one host + one peer,
│   │                       #   reliable (channel 0) / unreliable-sequenced
│   │                       #   (channel 1) send, connect/disconnect/packet
│   │                       #   callbacks. Knows nothing about Tetrisnow.
│   └── Protocol.*          # message enum + structs + manual byte-packed
│                           #   encode/decode (BoardSnapshot, LiveState,
│                           #   LinesClearedFx, AttackLandedFx, MatchReset,
│                           #   InputState, InputAction)
├── UI/                     # empty — Milestone 7 target (likely Dear ImGui)
└── Assets/Shaders/         # quad.vert / quad.frag (plain files, loaded
                             #   via TETRISNOW_ASSETS_DIR compile define)
```

**Hard rule the user cares about:** `Game/` must stay independent of
rendering. Color mappings, particle params, etc. all live in
`GameWindow.cpp`, never in `Game/`. `BlockType` itself carries no color.

## Key design decisions worth knowing

- **Rotation**: simplified (non-SRS) — 4 fixed orientations per piece +
  a small wall-kick attempt (offsets 0, ±1, ±2 columns). The user reported
  the rotation direction felt backwards once; it was flipped
  (`GameManager::rotateClockwise()` now calls `tryRotate(-1)`, not `+1`)
  and confirmed correct — **don't flip it back** without asking.
- **Held-key input**: movement/soft-drop **must** use per-frame polling
  (`glfwGetKey` + our own repeat timer in `GameWindow::pollHeldKey`), NOT
  GLFW's `GLFW_REPEAT` events — those come from OS keyboard-repeat timing
  and were unreliable with two players holding different keys at once.
  Only discrete actions (rotate, hard drop, reset) are in the GLFW key
  *callback*.
- **Two local players, one window (Local mode)**: both boards render
  side-by-side in one window with two local keysets: **P1 = arrows + Enter
  (hard drop)**, **P2 = WASD + Left Ctrl (hard drop)**. `R` resets the
  whole match. `Match` was deliberately designed to not know whether its
  two `Player`s are local or remote — Milestone 6 replaced P2's local
  keyset with network input in Host/Client mode without touching `Match`,
  `Player`, or `GameManager` at all (only `GameManager` gained one new
  notification callback, `onPieceLocked` — see below).
- **Snow attack tiers**: 1-2 lines → Snowball (power 1-2), 3 lines →
  SnowBomb (power 4), Tetris (4 lines) → Avalanche (power 6). Power =
  number of garbage rows sent. Garbage rows are mostly-filled with one
  random gap column each (classic multiplayer-Tetris "attack" model).
- **Snow energy**: tracked on `Player` but currently just an
  informational/cosmetic running total (`+10` per line cleared) — attacks
  fire immediately on clear rather than being banked/spent. A "charge and
  release" mechanic was explicitly *not* built (would be scope creep);
  revisit only if asked.
- **Particle rendering**: individual `drawQuad()` calls per particle (no
  GPU instancing). Deliberate simplicity — fine at this game's particle
  counts (capped at 2000). Would be the first thing to optimize if scaled
  up a lot.
- **Piece movement smoothing**: `GameManager::activePieceGeneration()`
  increments every time a *new* piece spawns; `GameWindow` uses it to
  distinguish "same piece moved" (ease smoothly) from "new piece spawned"
  (snap instantly) — otherwise pieces would visually slide from the old
  piece's last position into the new piece's spawn point.

## LAN multiplayer (Milestone 6) — how it actually works

Decided with the user up front: **ENet** (not raw sockets), and **manual
IP entry** for join (no LAN broadcast/auto-discovery — that's deferred,
see below). The model is **host-authoritative**:

- The **host** runs the exact same real `Match` (two real `GameManager`s)
  as Local mode — nothing about the simulation changes. Player 0 is
  always the host's own local player; player 1's input comes from the
  network instead of local WASD, fed into the *same* `pollHeldKey`/
  `GameManager` calls Local mode already used (see
  `GameWindow::processHeldInput` — it just picks `isDown` from
  `glfwGetKey` or from `m_remoteInput` depending on role).
- The **client** runs **no simulation at all** — it never touches
  `GameManager`. It only (a) samples its own local key state and sends it
  to the host every frame, and (b) renders both boards purely from
  messages the host sends it. This sidesteps any risk of the two sides'
  simulations drifting apart (gravity timing is real-time-based, so two
  independently-run simulations fed the same inputs at slightly different
  network-jittered moments *would* eventually disagree — host-authoritative
  avoids that class of bug entirely by only ever simulating once).
- Rendering was decoupled from `GameManager` via a small `GameWindow`-
  private `BoardView` struct (grid + active piece + generation +
  gameOver — no color, still rendering-agnostic). `GameWindow::boardView()`
  either captures one fresh from a live `GameManager` (Local/Host) or
  returns the client's network-populated mirror (Client). `drawSingleBoard`/
  `updatePieceSmoothing`/`drawInFlightAttacks` all consume `BoardView`
  uniformly now, regardless of role.
- Wire protocol (`Network/Protocol.h`) is deliberately **not** a per-frame
  full-board blob (the brief explicitly asked for event-based networking):
  a board's 200-cell grid only crosses the wire when it actually changes
  (`BoardSnapshot`, sent on lock, line-clear-with-garbage, or reset — hooked
  via the new `GameManager::onPieceLocked` callback plus the existing
  `onLinesCleared`/`onAttackLanded`). The one message sent every host tick,
  `LiveState`, carries only the two falling pieces' position/rotation/type
  and the (usually 0–1 entry) in-flight-attacks list — not board content.
  `LinesClearedFx`/`AttackLandedFx` carry just enough data to replay the
  exact same particle/camera-shake code on the client
  (`GameWindow::onLinesCleared`/`onAttackLanded` are called directly by the
  client's packet handler with host-sent data — the effect code itself
  isn't duplicated).
- Reliable channel (0): `BoardSnapshot`, `LinesClearedFx`, `AttackLandedFx`,
  `MatchReset`, `InputAction` (rotate/hard-drop/reset — one-shot, must not
  be dropped or duplicated). Unreliable-but-sequenced channel (1):
  `LiveState`, `InputState` (continuous per-tick state — a dropped packet
  is harmless since the next tick's packet supersedes it; sequencing means
  a late/stale one is discarded rather than applied out of order).
- **Verified live** (not just code-reviewed): ran an actual host process +
  client process over loopback, drove the client's piece with real
  `PostMessage` key events, and screenshotted both windows —the two
  screens were pixel-identical on all game state (stack + falling piece)
  while ambient-snow particle dust differed slightly, confirming two truly
  independent processes staying in sync. Also verified client-initiated
  `R` (reset) correctly resets both boards on both screens. See git log
  for this session if you need to reproduce the test script (PowerShell +
  `PrintWindow`/`PostMessage`, same technique as earlier milestones — no
  automated test was added to the repo).
- **Not built**: LAN auto-discovery (manual IP:port entry only, by
  agreement with the user), reconnect-after-disconnect, and any lobby/menu
  UI (there's no text rendering at all yet — Milestone 7 territory). A
  disconnect mid-match currently just stops that side's state from
  updating; nothing crashes, but there's no user-facing message beyond a
  stderr print.

## Controls (current build)

**Local two-player** (one window/keyboard):
- **Player 1**: ← → move, ↓ soft drop, ↑ rotate CW, Enter hard drop
- **Player 2**: A/D move, S soft drop, W rotate CW, Left Ctrl hard drop
- **R**: reset the match (both boards)

**Hosted/joined match** (`Tetrisnow.exe --host [port]` /
`--join <ip> [port]`, default port 7777): both sides use the Player-1
keyset (← → ↓ move/soft-drop, ↑ rotate CW, Enter hard drop) for their own
piece; `R` resets from either side.

## Suggested next steps (Milestone 7: UI and Polish)

Per the original brief/plan: menus (main menu, host/join screen replacing
the current CLI-args-only flow), an in-match HUD (score, snow energy,
next-piece preview — `ScoreSystem`/`Player::snowEnergy()` already track
the data, nothing currently renders it), and general visual polish. No
text-rendering solution exists yet at all — **Dear ImGui** was the
standing suggestion (pairs well with GLFW+OpenGL, immediate-mode fits this
game's simple-state-driven screens) but wasn't decided with the user;
propose it and confirm before pulling it in via `FetchContent`. This would
also be the natural place to replace the `--host`/`--join` CLI flags with
an actual host/join screen, and to surface connection status
(connecting/waiting-for-opponent/disconnected) instead of stderr prints.

## Workflow notes for whoever continues

- The user wants **each milestone explained, key files shown, tested, and
  approved before moving to the next** (per the original prompt) — don't
  batch multiple milestones without checking in.
- **Commit after each milestone** (and promptly after any fix) — earlier
  in this project, uncommitted Milestone 5 work was wiped by what looked
  like an external `git clean`/`checkout` mid-session. Committing early
  and often is the only real protection against that.
- Attribution line for commits: `Co-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>`.
- The user is hands-on and will actually build/run things themselves too
  — keep the README's build instructions in sync with reality.
