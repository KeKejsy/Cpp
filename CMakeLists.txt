cmake_minimum_required(VERSION 3.15)
project(RhythmMaster)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SFML 3 REQUIRED COMPONENTS Graphics Audio Window System)

add_executable(rhythm_master
    src/main.cpp
    src/GameFramework.cpp
)

target_link_libraries(rhythm_master
    SFML::Graphics
    SFML::Audio
    SFML::Window
    SFML::System
)
