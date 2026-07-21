
add_library(ModelLoader STATIC model_loader.cpp)
target_link_libraries(ModelLoader PUBLIC SynoraEngine)
target_include_directories(ModelLoader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(InputSystem input_system.cpp)
target_link_libraries(InputSystem PRIVATE SynoraEngine)

add_executable(GLTest gl_test.cpp)
target_link_libraries(GLTest PRIVATE SynoraEngine)

add_executable(GLTest2 gl_test2.cpp)
target_link_libraries(GLTest2 PRIVATE SynoraEngine)

add_executable(GLRenderTest gl_renderer.cpp)
target_link_libraries(GLRenderTest PRIVATE ModelLoader)
