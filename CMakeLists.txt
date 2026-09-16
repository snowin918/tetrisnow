cmake_minimum_required(VERSION 3.21)
project(Tetrisnow LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt's meta-object compiler / UI compiler / resource compiler.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# OpenGL is a separate Qt6 module (QOpenGLFunctions_3_3_Core,
# QOpenGLShaderProgram, QOpenGLTexture all live here).
find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets OpenGL)

# GLM (header-only math library) fetched at configure time so contributors
# don't need a separate install step. Requires network access on first
# configure; see README.md for a vcpkg alternative if that's not available.
include(FetchContent)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

add_executable(Tetrisnow
    main.cpp
    UI/MainWindow.h
    UI/MainWindow.cpp
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
    Engine/GLMQt.h
    Assets/assets.qrc
)

target_include_directories(Tetrisnow PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(Tetrisnow PRIVATE
    Qt6::Widgets
    Qt6::OpenGLWidgets
    Qt6::OpenGL
    glm::glm
)

# Keep the console attached for now so qDebug()/std::cout output is visible
# during development. We can flip this to a real GUI app (no console) once
# the project nears release.
