
add_executable(InputSystem input_system.cpp)
target_link_libraries(InputSystem PRIVATE Synora)

add_executable(GLTest gl_test.cpp)
target_link_libraries(GLTest PRIVATE Synora)

