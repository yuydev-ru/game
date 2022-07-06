cmake_minimum_required(VERSION 3.23)
project (FullGame)
set (CMAKE_CXX_STANDARD 11)

add_executable (Game Source/game.cpp)

add_subdirectory (Dependencies)

target_link_libraries (Game LINK_PRIVATE Engine)
