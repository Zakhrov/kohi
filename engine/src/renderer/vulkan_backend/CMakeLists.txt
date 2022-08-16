project(VulkanRendererBackend)
add_library(${PROJECT_NAME} SHARED vulkan_backend.cpp
    vulkan_device.cpp
    vulkan_image.cpp
    vulkan_swapchain.cpp
    vulkan_renderpass.cpp
    vulkan_command_buffer.cpp
    vulkan_framebuffer.cpp
    vulkan_fence.cpp
    vulkan_utils.cpp
    )
target_link_libraries(${PROJECT_NAME} LINK_PUBLIC vulkan)