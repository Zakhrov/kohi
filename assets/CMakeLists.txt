project(KohiAssets)

find_package(Vulkan COMPONENTS glslc glslangValidator)
find_program(Vulkan_GLSLC_EXECUTABLE NAMES glslc HINTS Vulkan::glslc)
find_program(Vulkan_GLSL_VALIDATOR NAMES glslangValidator HINTS Vulkan::glslangValidator)
file(GLOB_RECURSE GLSL_FRAG_SHADER_FILES
    "shaders/*.frag"
    
    
    )

file(GLOB_RECURSE GLSL_VERT_SHADER_FILES
    "shaders/*.vert"
    )

file(GLOB_RECURSE TEXTURE_FILES "textures/*.png")
file(GLOB_RECURSE MATERIAL_FILES "materials/*.kmt")

foreach(GLSL_FRAG ${GLSL_FRAG_SHADER_FILES})
  get_filename_component(FILE_NAME ${GLSL_FRAG} NAME)
  set(SPIRV "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
    COMMAND ${Vulkan_GLSLC_EXECUTABLE} -fshader-stage=frag ${GLSL_FRAG}  -o ${SPIRV}
    DEPENDS ${GLSL_FRAG})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL_FRAG)

foreach(GLSL_VERT ${GLSL_VERT_SHADER_FILES})
  get_filename_component(FILE_NAME ${GLSL_VERT} NAME)
  set(SPIRV "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
    COMMAND ${Vulkan_GLSLC_EXECUTABLE} -fshader-stage=vert ${GLSL_VERT}  -o ${SPIRV}
    DEPENDS ${GLSL_VERT})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL_VERT)

foreach(TEXTURE_FILE ${TEXTURE_FILES})
  get_filename_component(FILE_NAME ${TEXTURE_FILE} NAME)
  set(SPIRV "${PROJECT_BINARY_DIR}/textures/${FILE_NAME}")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/textures/"
    COMMAND cp ${TEXTURE_FILE} ${SPIRV}
    DEPENDS ${GLSL_FRAG})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(TEXTURE_FILE)


foreach(MATERIAL_FILE ${MATERIAL_FILES})
  get_filename_component(FILE_NAME ${MATERIAL_FILE} NAME)
  set(SPIRV "${PROJECT_BINARY_DIR}/materials/${FILE_NAME}")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/materials/"
    COMMAND cp ${MATERIAL_FILE} ${SPIRV}
    DEPENDS ${GLSL_FRAG})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(MATERIAL_FILE)

add_custom_target(
    ${PROJECT_NAME} 
    DEPENDS ${SPIRV_BINARY_FILES}
    )
