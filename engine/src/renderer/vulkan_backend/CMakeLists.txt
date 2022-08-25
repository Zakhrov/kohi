project(KohiVulkanBackend)
find_package(glm REQUIRED)

add_library(${PROJECT_NAME} SHARED )
target_sources(${PROJECT_NAME} PRIVATE vulkan_backend.cpp
vulkan_device.cpp
vulkan_image.cpp
vulkan_swapchain.cpp
vulkan_renderpass.cpp
vulkan_command_buffer.cpp
vulkan_framebuffer.cpp
vulkan_fence.cpp
vulkan_platform_linux.cpp
vulkan_utils.cpp
vulkan_shader_utils.cpp
shaders/vulkan_object_shader.cpp
)

target_link_libraries(${PROJECT_NAME} LINK_PUBLIC vulkan glm::glm)