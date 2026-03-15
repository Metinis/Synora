set(CMAKE_CXX_STANDARD 23)

find_package(Vulkan REQUIRED)

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

add_executable(PuzzleEngine
        ${ENGINE_SOURCES}
)
target_precompile_headers(PuzzleEngine
        PUBLIC
        "$<$<COMPILE_LANGUAGE:CXX>:${PROJECT_SOURCE_DIR}/include/PuzzleEngine/synpch.h>"
)
target_include_directories(PuzzleEngine
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
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
  VulkanMemoryAllocator
  glm::glm
)

# This is to make the projection matrices in glm work with vulkan
target_compile_definitions(PuzzleEngine PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE GLFW_INCLUDE_VULKAN ${APPLE_COMPILE_DEFINITIONS})

