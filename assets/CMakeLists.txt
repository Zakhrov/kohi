project(KohiAssets)

find_package(Vulkan COMPONENTS glslc glslangValidator)
find_program(Vulkan_GLSLC_EXECUTABLE NAMES glslc HINTS Vulkan::glslc)
find_program(Vulkan_GLSL_VALIDATOR NAMES glslangValidator HINTS Vulkan::glslangValidator)
file(GLOB_RECURSE GLSL_SOURCE_FILES
    "shaders/*.frag"
    "shaders/*.vert"
    )

foreach(GLSL ${GLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
    COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    ${PROJECT_NAME} 
    DEPENDS ${SPIRV_BINARY_FILES}
    )
