set(CMAKE_CXX_STANDARD 23)

set (APPLE_COMPILE_DEFINITIONS)

if(APPLE)
    # This is to enable VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME to make moltenvk work
    set(APPLE_COMPILE_DEFINITIONS VK_ENABLE_BETA_EXTENSIONS)
endif()

file(GLOB_RECURSE ENGINE_SOURCES
        CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_library(SynoraEngine STATIC ${ENGINE_SOURCES})

target_precompile_headers(
  SynoraEngine
  PUBLIC
  "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/../include/SynoraEngine/synpch.h>"
)
target_include_directories(
  SynoraEngine
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
  ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/stb
  ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/imgui
  ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/imgui/backends
  ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/tinyfiledialogs
)
target_link_libraries(
  SynoraEngine
  PUBLIC
  glfw
  spdlog
  miniaudio
  EnTT::EnTT
  Jolt
  Vulkan::Vulkan
  GPUOpen::VulkanMemoryAllocator
  glm::glm
  assimp
  imgui
  tinyfiledialogs
)

# This is to make the projection matrices in glm work with vulkan
target_compile_definitions(SynoraEngine PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_LEFT_HANDED GLFW_INCLUDE_VULKAN ${APPLE_COMPILE_DEFINITIONS})
