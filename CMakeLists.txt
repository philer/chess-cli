cmake_minimum_required(VERSION 3.10)

set(CMAKE_EXPORT_COMPILE_COMMANDS YES)

project(ChessCLI VERSION 1.0)

add_definitions("-Wall")
add_definitions("-Wextra")
add_definitions("-Wno-parentheses")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(chess src/chess.cpp)

target_include_directories(chess PUBLIC "${PROJECT_BINARY_DIR}")
