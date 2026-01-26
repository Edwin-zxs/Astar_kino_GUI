cmake_minimum_required(VERSION 3.16)

project(Astar_kino_GUI VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Gui)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
)

add_executable(Astar_kino_GUI ${PROJECT_SOURCES})

target_link_libraries(Astar_kino_GUI PRIVATE Qt6::Widgets Qt6::Gui)

# Handle MacOS bundle creation
if(APPLE)
    set_target_properties(Astar_kino_GUI PROPERTIES MACOSX_BUNDLE ON)
endif()