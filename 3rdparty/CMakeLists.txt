
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)

set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
add_subdirectory(spdlog)

add_subdirectory(miniaudio)

add_subdirectory(entt)

set(TARGET_HELLO_WORLD OFF CACHE BOOL "" FORCE)
set(TARGET_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(TARGET_PERFORMANCE_TEST OFF CACHE BOOL "" FORCE)
set(TARGET_SAMPLES OFF CACHE BOOL "" FORCE)
set(TARGET_VIEWER OFF CACHE BOOL "" FORCE)

# Compile Jolt successfully on Linux
# Jolt needs Vulkan for some hair physics thing I think.
if(UNIX AND NOT APPLE)
set(Vulkan_INCLUDE_DIR "$ENV{VULKAN_SDK}/include")
set(Vulkan_LIBRARY "$ENV{VULKAN_SDK}/lib/libvulkan.so")
endif()

add_subdirectory(JoltPhysics/Build)
