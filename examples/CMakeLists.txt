
add_executable(InputSystem input_system.cpp)
target_link_libraries(InputSystem PRIVATE Synora)

add_executable(GLTest gl_test.cpp)
target_link_libraries(GLTest PRIVATE Synora)
target_include_directories(GLTest PRIVATE ${PROJECT_SOURCE_DIR}/3rdparty/stb)
target_compile_options(GLTest PRIVATE -g)

