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

FetchContent_MakeAvailable(glfw glm)

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
    Engine/OpenGLLoader.h
    Engine/OpenGLLoader.cpp
)

target_include_directories(Tetrisnow PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(Tetrisnow PRIVATE
    glfw
    glm::glm
    OpenGL::GL
)

# Shaders are loaded from disk at runtime (no resource-embedding step), so
# point the binary at the source tree's Assets/ folder regardless of the
# working directory it's launched from.
target_compile_definitions(Tetrisnow PRIVATE
    TETRISNOW_ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Assets"
)
