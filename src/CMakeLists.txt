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

list(FILTER ENGINE_SOURCES EXCLUDE REGEX "cmake-build")

add_library(Synora STATIC ${ENGINE_SOURCES})
target_precompile_headers(
  Synora
  PUBLIC
  "$<$<COMPILE_LANGUAGE:CXX>:${PROJECT_SOURCE_DIR}/include/PuzzleEngine/synpch.h>"
)
target_include_directories(
  Synora 
  PUBLIC 
  ${PROJECT_SOURCE_DIR}/include
  PRIVATE 
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_SOURCE_DIR}/3rdparty/stb
  ${PROJECT_SOURCE_DIR}/3rdparty/imgui
  ${PROJECT_SOURCE_DIR}/3rdparty/imgui/backends
)
target_link_libraries(
  Synora 
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
)

add_executable(PuzzleEngine
        ${ENGINE_SOURCES}
)
target_precompile_headers(PuzzleEngine
        PUBLIC
        "$<$<COMPILE_LANGUAGE:CXX>:${PROJECT_SOURCE_DIR}/include/PuzzleEngine/synpch.h>"
)
target_include_directories(PuzzleEngine
  PUBLIC 
  ${PROJECT_SOURCE_DIR}/include
  PRIVATE
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_SOURCE_DIR}/3rdparty/stb
)

target_link_libraries(
  PuzzleEngine 
  PRIVATE
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
)

# This is to make the projection matrices in glm work with vulkan
target_compile_definitions(PuzzleEngine PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_LEFT_HANDED GLFW_INCLUDE_VULKAN ${APPLE_COMPILE_DEFINITIONS})
