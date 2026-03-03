set(CMAKE_CXX_STANDARD 23)

add_executable(
  PuzzleEngine
  main.cpp
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
