project(KohiVulkanShaders)

add_library(${PROJECT_NAME} SHARED )
target_sources(${PROJECT_NAME} PRIVATE vulkan_material_shader.cpp)
target_link_libraries(${PROJECT_NAME} LINK_PUBLIC vulkan KohiMathLibrary)