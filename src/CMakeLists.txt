set(CMAKE_CXX_STANDARD 23)

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
        "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/synpch.h>"
)
target_include_directories(PuzzleEngine
        PRIVATE
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
)
