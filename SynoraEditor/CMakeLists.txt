cmake_minimum_required(VERSION 4.1)

# Executable
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# DLLs
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Static libraries
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# For me and my Neovim buddy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(SynoraEditor
        src/main.cpp
)

if(NOT TARGET SynoraEngine)

    message(STATUS "SynoraEngine not found, fetching...")

    include(FetchContent)

    FetchContent_Declare(
            SynoraEngine
            GIT_REPOSITORY https://github.com/metinis/SynoraEngine.git
            GIT_TAG main
    )

    FetchContent_MakeAvailable(SynoraEngine)

else()
    message(STATUS "Using existing SynoraEngine target")
endif()

target_link_libraries(SynoraEditor
        PRIVATE SynoraEngine
)

add_subdirectory(resources)