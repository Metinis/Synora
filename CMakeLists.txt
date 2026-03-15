cmake_minimum_required(VERSION 4.1)

project(PuzzleEngine CXX)

# Executable
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# DLLs
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Static libraries
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# For me and my Neovim buddy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(3rdparty)
add_subdirectory(src)
add_subdirectory(resources)
