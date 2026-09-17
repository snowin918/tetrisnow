cmake_minimum_required(VERSION 3.21)
project(Tetrisnow LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)

# GLFW (windowing/input/context) and GLM (math) are both small, fast-building
# dependencies fetched at configure time — no separate SDK install required.
include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)

# ENet (reliable/unreliable UDP) powers Milestone 6's LAN link — a small C
# library built for exactly this, fetched the same way as GLFW/GLM rather
# than adding a heavier networking SDK.
set(ENET_STATIC ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    enet
    GIT_REPOSITORY https://github.com/lsalzman/enet.git
    GIT_TAG v1.3.18
)

# Dear ImGui (Milestone 7's menus/HUD) ships no CMakeLists.txt of its own —
# FetchContent just populates the source, and its .cpp files are added to
# our own target below alongside the GLFW+OpenGL3 backends it provides for
# exactly this windowing/rendering combination.
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.0
)

# stb_image (Phase 7's sprite-character art loading) — a header-only
# single-file library with no CMakeLists.txt of its own, same situation as
# Dear ImGui: FetchContent just populates the source, and Engine/
# StbImageImpl.cpp provides its implementation in exactly one translation
# unit (its own documented usage pattern). stb has no version tags, so this
# pins to a specific commit rather than floating on a branch.
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG 2c980bb59875b0d32144a71867fbdebb2f77cd20 # master as of this pin
)

# Dlib (Phase 2's classical CV: face detection + the 68-point face-landmark
# shape predictor) covers both halves of the brief's "OpenCV + Dlib"
# alternative on its own — the detector and shape predictor are dlib APIs,
# and OpenCV's role would only have been image I/O, which Engine/
# TextureManager + stb_image already handle. Skipping OpenCV here cuts a
# second heavy from-source build for no functional loss.
#
# All of dlib's optional heavy bits are switched off: no GUI (X11/Win32
# widgets we don't use), no CUDA, no external BLAS/LAPACK (falls back to
# its own, fine for occasional single-face detection rather than training),
# no bundled image codecs (we feed it raw pixel arrays via stb_image
# instead of dlib's own jpeg/png loaders). This keeps the build to just
# dlib's core numeric/image-processing/ML code.
set(DLIB_NO_GUI_SUPPORT ON CACHE BOOL "" FORCE)
set(DLIB_USE_CUDA OFF CACHE BOOL "" FORCE)
set(DLIB_USE_BLAS OFF CACHE BOOL "" FORCE)
set(DLIB_USE_LAPACK OFF CACHE BOOL "" FORCE)
set(DLIB_USE_MKL_FFT OFF CACHE BOOL "" FORCE)
set(DLIB_JPEG_SUPPORT OFF CACHE BOOL "" FORCE)
set(DLIB_PNG_SUPPORT OFF CACHE BOOL "" FORCE)
set(DLIB_GIF_SUPPORT OFF CACHE BOOL "" FORCE)
set(DLIB_WEBP_SUPPORT OFF CACHE BOOL "" FORCE)
set(DLIB_LINK_WITH_SQLITE3 OFF CACHE BOOL "" FORCE)
set(DLIB_USE_FFMPEG OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    dlib
    GIT_REPOSITORY https://github.com/davisking/dlib.git
    GIT_TAG v19.24.6
)

FetchContent_MakeAvailable(glfw glm enet imgui stb dlib)

add_executable(Tetrisnow
    main.cpp
    Engine/GameWindow.h
    Engine/GameWindow.cpp
    Engine/Camera.h
    Engine/Camera.cpp
    Engine/Renderer.h
    Engine/Renderer.cpp
    Engine/ShaderManager.h
    Engine/ShaderManager.cpp
    Engine/TextureManager.h
    Engine/TextureManager.cpp
    Engine/StbImageImpl.cpp
    Engine/OpenGLLoader.h
    Engine/OpenGLLoader.cpp
    Engine/BlockColors.h
    Engine/ImGuiConfig.h
    Engine/ParticleSystem.h
    Engine/ParticleSystem.cpp
    Engine/EffectManager.h
    Engine/EffectManager.cpp
    Engine/CharacterAsset.h
    Engine/CharacterRenderer.h
    Engine/CharacterRenderer.cpp
    Engine/SpriteCharacterAsset.h
    Engine/SpriteCharacterAsset.cpp
    Engine/AnimationSystem.h
    Engine/AnimationSystem.cpp
    FaceAvatar/FaceAvatarSystem.h
    FaceAvatar/FaceAvatarSystem.cpp
    FaceAvatar/FaceLandmarks.h
    FaceAvatar/FaceLandmarkDetector.h
    FaceAvatar/FaceLandmarkDetector.cpp
    FaceAvatar/FaceMesh.h
    FaceAvatar/FaceMesh.cpp
    FaceAvatar/FaceEmotion.h
    FaceAvatar/FaceExpressionController.h
    FaceAvatar/FaceExpressionController.cpp
    FaceAvatar/GameEvent.h
    Game/BlockType.h
    Game/Board.h
    Game/Board.cpp
    Game/CharacterEmotion.h
    Game/CharacterController.h
    Game/CharacterController.cpp
    Game/Tetromino.h
    Game/Tetromino.cpp
    Game/ScoreSystem.h
    Game/ScoreSystem.cpp
    Game/GameManager.h
    Game/GameManager.cpp
    Game/SnowAttack.h
    Game/SnowAttack.cpp
    Game/SnowEnergy.h
    Game/StatusEffect.h
    Game/StatusEffects.h
    Game/StatusEffects.cpp
    Game/Player.h
    Game/Player.cpp
    Game/Match.h
    Game/Match.cpp
    Network/Protocol.h
    Network/Protocol.cpp
    Network/NetworkSession.h
    Network/NetworkSession.cpp
    UI/MenuScreens.h
    UI/MenuScreens.cpp
    UI/Hud.h
    UI/Hud.cpp
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(Tetrisnow PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${enet_SOURCE_DIR}/include
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${stb_SOURCE_DIR}
)

target_link_libraries(Tetrisnow PRIVATE
    glfw
    glm::glm
    OpenGL::GL
    enet
    dlib::dlib
)

if(WIN32)
    target_link_libraries(Tetrisnow PRIVATE ws2_32 winmm)
endif()

# Shaders are loaded from disk at runtime (no resource-embedding step), so
# point the binary at the source tree's Assets/ folder regardless of the
# working directory it's launched from.
target_compile_definitions(Tetrisnow PRIVATE
    TETRISNOW_ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Assets"
    # Dear ImGui's OpenGL3 backend would otherwise bundle its own gl3w-
    # derived loader, which declares GL function-pointer globals under the
    # same names Engine/OpenGLLoader.h already does (e.g. `glGenBuffers`) —
    # linking both would fail with duplicate symbols. This makes imgui.h
    # include our own config, which points the backend at our loader
    # instead (see Engine/ImGuiConfig.h).
    "IMGUI_USER_CONFIG=\"${CMAKE_CURRENT_SOURCE_DIR}/Engine/ImGuiConfig.h\""
)
