
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

add_subdirectory(JoltPhysics/Build)

add_subdirectory(glm)
add_subdirectory(VulkanMemoryAllocator)

add_library(glad STATIC glad/src/glad.c)
target_include_directories(
  glad 
  PUBLIC 
  glad/include
)
