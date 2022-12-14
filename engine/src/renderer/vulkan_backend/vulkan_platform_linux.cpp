#define VK_USE_PLATFORM_XCB_KHR
#include "renderer/vulkan_backend/vulkan_platform.h"
#include <vulkan/vulkan.h>
#include <xcb/xcb.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>  // sudo apt-get install libx11-dev
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>  // sudo apt-get install libxkbcommon-x11-dev
#include "core/logger.h"

void platform_get_required_extension_names(std::vector<const char*>* extensions){
    extensions->emplace_back("VK_KHR_xcb_surface");


}
b8 platform_create_vulkan_surface(PlatformState* platformState,VulkanContext* context){
    if(!platformState){
        KFATAL("Vulkan surface creation failed.");
        return false;
    }
    


    VkXcbSurfaceCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR};
    
    createInfo.connection = platformState->connection;
    createInfo.window = platformState->window;

    VkResult result = vkCreateXcbSurfaceKHR(
        context->instance,
        &createInfo,
        context->allocator,
        &platformState->surface);
    if (result != VK_SUCCESS) {
        KFATAL("Vulkan surface creation failed.");
        return false;
    }

    context->surface = platformState->surface;
    return true;


}