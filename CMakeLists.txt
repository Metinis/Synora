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

option(SYNORA_BUILD_EXAMPLES "Build Synora examples" ON)

# Prefer the Vulkan SDK if available
if(DEFINED ENV{VULKAN_SDK})
    message(STATUS "Using Vulkan SDK at $ENV{VULKAN_SDK}")

    set(Vulkan_INCLUDE_DIR "$ENV{VULKAN_SDK}/include")

    if(WIN32)
        set(Vulkan_LIBRARY "$ENV{VULKAN_SDK}/Lib/vulkan-1.lib")
    elseif(APPLE)
        set(Vulkan_LIBRARY "$ENV{VULKAN_SDK}/lib/libvulkan.1.dylib")
    else()
        set(Vulkan_LIBRARY "$ENV{VULKAN_SDK}/lib/libvulkan.so")
    endif()

else()
    # Fallback to system Vulkan
    find_package(Vulkan REQUIRED)
    set(Vulkan_INCLUDE_DIR ${Vulkan_INCLUDE_DIRS})
    set(Vulkan_LIBRARY ${Vulkan_LIBRARIES})
endif()

add_subdirectory(3rdparty)
add_subdirectory(src)

if (SYNORA_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

add_subdirectory(resources)
