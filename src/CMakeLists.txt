set(CMAKE_CXX_STANDARD 23)

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
target_include_directories(Synora PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(
  Synora 
  PUBLIC
  glfw
  spdlog
  miniaudio
  EnTT::EnTT
  Jolt
)

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
