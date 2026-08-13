cmake_minimum_required(VERSION 3.3)

project(SynoraWorkspace)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(SynoraEngine)
add_subdirectory(SynoraEditor)
add_subdirectory(SynoraSandbox)

set(SANDBOX_DIR "${CMAKE_SOURCE_DIR}/SynoraSandbox")

message(STATUS "Sandbox dir: ${SANDBOX_DIR}")

get_property(
        ALL_TARGETS
        DIRECTORY ${SANDBOX_DIR}
        PROPERTY BUILDSYSTEM_TARGETS
)

foreach(TARGET ${ALL_TARGETS})
    add_dependencies(SynoraEditor ${TARGET})
endforeach()
