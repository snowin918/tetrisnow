cmake_minimum_required(VERSION 3.21)
project(Tetrisnow LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt's meta-object compiler / UI compiler / resource compiler.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets)

add_executable(Tetrisnow
    main.cpp
    UI/MainWindow.h
    UI/MainWindow.cpp
    Engine/GameWindow.h
    Engine/GameWindow.cpp
)

target_include_directories(Tetrisnow PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(Tetrisnow PRIVATE
    Qt6::Widgets
    Qt6::OpenGLWidgets
)

# Keep the console attached for now so qDebug()/std::cout output is visible
# during development. We can flip this to a real GUI app (no console) once
# the project nears release.
