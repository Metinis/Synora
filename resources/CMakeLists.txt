set(CMAKE_CXX_STANDARD 23)

find_package(Vulkan REQUIRED COMPONENTS glslc)
get_target_property(GLSLC Vulkan::glslc IMPORTED_LOCATION)

set(SHADERS_SRC_DIR "${CMAKE_SOURCE_DIR}/resources/shaders")
set(SHADERS_BIN_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/generated/shaders")

file(GLOB SHADERS
	"${SHADERS_SRC_DIR}/*.vert"
	"${SHADERS_SRC_DIR}/*.frag"
	"${SHADERS_SRC_DIR}/*.comp"
	"${SHADERS_SRC_DIR}/*.geom"
	"${SHADERS_SRC_DIR}/*.tesc"
	"${SHADERS_SRC_DIR}/*.tese")

file(MAKE_DIRECTORY "${SHADERS_BIN_DIR}")

foreach(SHADER ${SHADERS})
	get_filename_component(SHADER_NAME ${SHADER} NAME)
	set(SHADER_BIN_NAME "${SHADERS_BIN_DIR}/${SHADER_NAME}.spv")
	add_custom_command(
			OUTPUT ${SHADER_BIN_NAME}
			COMMAND ${GLSLC} ${SHADER} -o ${SHADER_BIN_NAME}
			DEPENDS ${SHADER}
			COMMENT "Compiling ${SHADER_NAME}"
			VERBATIM
	)
	list(APPEND SPV_SHADERS "${SHADER_BIN_NAME}")
endforeach()

add_custom_target(build_shaders ALL DEPENDS ${SPV_SHADERS})

add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/resources/textures"
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/textures"
    COMMENT "Copying textures to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/textures"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/resources/shaders"
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/shaders"
    COMMENT "Copying shaders to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/resources/assets/"
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/assets"
    COMMENT "Copying assets to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/assets")

add_dependencies(PuzzleEngine build_shaders)
add_dependencies(PuzzleEngine copy_assets)
