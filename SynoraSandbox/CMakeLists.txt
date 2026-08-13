cmake_minimum_required(VERSION 3.3)

project(SynoraSandbox)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(BUILD_MAIN "Build the main application" ON)

file(GLOB_RECURSE SYSTEM_SOURCES
        CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/systems/*.cpp
)


if(NOT TARGET SynoraEngine)

    include(FetchContent)

    FetchContent_Declare(
            SynoraEngine
            GIT_REPOSITORY https://github.com/metinis/SynoraEngine.git
            GIT_TAG main
    )

    FetchContent_MakeAvailable(SynoraEngine)

endif()

foreach(SYSTEM_FILE ${SYSTEM_SOURCES})

    get_filename_component(SYSTEM_NAME ${SYSTEM_FILE} NAME_WE)
    get_filename_component(SYSTEM_DIR ${SYSTEM_FILE} DIRECTORY)

    add_library(${SYSTEM_NAME} SHARED
            ${SYSTEM_FILE}
    )

    set_target_properties(${SYSTEM_NAME} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY ${SYSTEM_DIR}
    )

    if(WIN32)
        set_target_properties(${SYSTEM_NAME} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${SYSTEM_DIR}
        )

        target_compile_definitions(${SYSTEM_NAME}
                PRIVATE
                SYN_SYSTEM_DLL
        )
    endif()

    target_include_directories(${SYSTEM_NAME}
            PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/systems
    )

    target_link_libraries(${SYSTEM_NAME}
            PRIVATE
            SynoraEngine
    )
endforeach()

# Building standalone game
if(BUILD_MAIN)
    add_executable(SynoraSandbox main.cpp)
    target_link_libraries(SynoraSandbox PRIVATE SynoraEngine)
    add_definitions(-DPROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")
endif()


