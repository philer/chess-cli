cmake_minimum_required(VERSION 3.10)

project(LearnCPP VERSION 1.0)

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
