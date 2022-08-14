#include "vulkan_backend.h"
#include "vulkan_types.inl"
#include "../../core/logger.h"
#include "../../containers/darray.h"
#include "vulkan_platform.h"
#include <string.h>

static VulkanContext context{};

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void* userData);


b8 vulkan_renderer_backend_initialize(RendererBackend* backend, const char* applicationName, struct PlatformState* platformState){
    //TODO: Custom Allocator
    context.allocator = VK_NULL_HANDLE;
    VkApplicationInfo appInfo{};
    appInfo.pApplicationName = applicationName;
    appInfo.apiVersion = VK_API_VERSION_1_2;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Kohi Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    std::vector<const char*> extensions = std::vector<const char*>(0);
    std::vector<const char*> requiredLayers = std::vector<const char*>(0);

#ifndef NDEBUG
    extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
    
    
    extensions.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);
    platform_get_required_extension_names(&extensions);
     for(int i=0; i < extensions.size(); i++){
                KDEBUG("Enabled extensions are >>> %s",extensions[i]);

        }
#ifndef NDEBUG
    KINFO(" Loading Validation layers");
    requiredLayers.emplace_back("VK_LAYER_KHRONOS_validation");
    u32 availableLayerCount = 0;

    VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayerCount,VK_NULL_HANDLE));
    std::vector<VkLayerProperties> availableLayers = std::vector<VkLayerProperties>(availableLayerCount);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayerCount,availableLayers.data()));
    for(int i=0; i < requiredLayers.size(); i++){
        KINFO("Searching for layer: %s ...",requiredLayers[i]);
        b8 found = FALSE;
        for(int j=0; j< availableLayerCount; j++){
            KINFO("Available layer: %s",availableLayers[j].layerName);
            if(strcmp(requiredLayers[i],availableLayers[j].layerName) == 0 ){
                found = TRUE;
                KINFO("Found %s",requiredLayers[i]);
                break;
            }
        }
        if(!found){
            KFATAL("Required validation layer %s is missing",requiredLayers[i]);
            return FALSE;
        }
    }
    KINFO("All required validation layers found");
#endif
    VkInstanceCreateInfo createInfo{};
    createInfo.pApplicationInfo = &appInfo;
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledExtensionCount = extensions.size();
    createInfo.enabledLayerCount = requiredLayers.size();
    createInfo.ppEnabledLayerNames = requiredLayers.data();

    
    VK_CHECK(vkCreateInstance(&createInfo,context.allocator,&context.instance));
    KINFO("Vulkan Instance created");

#ifndef NDEBUG
    KDEBUG("Creating Vulkan Debugger");
    u32 logSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity = logSeverity;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debugCreateInfo.pfnUserCallback = vk_debug_callback;
    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkCreateDebugUtilsMessengerEXT");
    KASSERT_MSG(func, "Failed to create debug messenger!");
    VK_CHECK(func(context.instance, &debugCreateInfo, context.allocator, &context.debugMessenger));
    KDEBUG("Vulkan debugger created.");
#endif

    KINFO("Vulkan backend initialized successfully");
    return TRUE;
    

}
void vulkan_renderer_backend_shutdown(RendererBackend* backend){

    
#ifndef NDEBUG
    if (context.debugMessenger != nullptr) {
        KDEBUG("Destroying Vulkan debugger...");
        PFN_vkDestroyDebugUtilsMessengerEXT func =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkDestroyDebugUtilsMessengerEXT");
        func(context.instance, context.debugMessenger, context.allocator);
    }
#endif

    KINFO("Destroying Vulkan instance...");
    vkDestroyInstance(context.instance, context.allocator);

}

void vulkan_renderer_backend_on_resized(RendererBackend* backend, u16 width, u16 height){

}

b8 vulkan_renderer_backend_begin_frame(RendererBackend* backend, f64 deltaTime){
    return TRUE;

}
b8 vulkan_renderer_backend_end_frame(RendererBackend* backend, f64 deltaTime){
    return TRUE;

}

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,VkDebugUtilsMessageTypeFlagsEXT messageTypes,const VkDebugUtilsMessengerCallbackDataEXT* callbackData,void* userData){
        switch (messageSeverity) {
        default:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            KERROR(callbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            KWARN(callbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            KINFO(callbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            KTRACE(callbackData->pMessage);
            break;
    }
    return VK_FALSE;
}
