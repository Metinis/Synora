
add_executable(InputSystem input_system.cpp)
target_link_libraries(InputSystem PRIVATE SynoraEngine glfw)

add_executable(GLTest gl_test.cpp)
target_link_libraries(GLTest PRIVATE SynoraEngine glfw)

add_executable(GLTest2 gl_test2.cpp)
target_link_libraries(GLTest2 PRIVATE SynoraEngine assimp glfw)

add_executable(GLRenderTest gl_renderer.cpp)
target_link_libraries(GLRenderTest PRIVATE SynoraEngine glfw)
