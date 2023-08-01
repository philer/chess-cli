cmake_minimum_required(VERSION 3.10)

set(CMAKE_EXPORT_COMPILE_COMMANDS YES)

project(LearnCPP VERSION 1.0)

# set(CMAKE_C_COMPILER "clang")
# set(CMAKE_CXX_COMPILER "clang++")
# set(CMAKE_CXX_EXTENSIONS OFF)  # gnu++ -> c++
# add_definitions("-fexperimental-library")

add_definitions("-Wall")
add_definitions("-Wextra")
add_definitions("-Wno-parentheses")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# configure_file(ChessConfig.h.in ChessConfig.h)

# add_subdirectory(src)

add_executable(hello src/hello.cpp)
add_executable(chess src/chess.cpp)
add_executable(set src/set.cpp)

# target_link_libraries

# target_include_directories()
# Hint: ${PROJECT_SOURCE_DIR} is a path to the project source. AKA This folder!

target_include_directories(chess PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           )
