#include "vulkan_backend.h"
#include "vulkan_types.inl"
#include "../../core/logger.h"
#include "../../containers/darray.h"
#include "vulkan_platform.h"
#include "vulkan_device.h"
#include <string.h>
#include "vulkan_swapchain.h"
#include "vulkan_renderpass.h"
#include "vulkan_command_buffer.h"


static VulkanContext context{};

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void* userData);


i32 find_memory_index(u64 memoryTypeBits,VkMemoryPropertyFlags memoryFlags,int deviceIndex);

void create_command_buffers(RendererBackend *backend,int deviceIndex);

b8 vulkan_renderer_backend_initialize(RendererBackend* backend, const char* applicationName, struct PlatformState* platformState){
    //TODO: Custom Allocator
    context.findMemoryIndex = find_memory_index;
    context.allocator = VK_NULL_HANDLE;
    VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
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
    VkInstanceCreateInfo createInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
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
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debugCreateInfo.messageSeverity = logSeverity;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debugCreateInfo.pfnUserCallback = vk_debug_callback;
    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkCreateDebugUtilsMessengerEXT");
    KASSERT_MSG(func, "Failed to create debug messenger!");
    VK_CHECK(func(context.instance, &debugCreateInfo, context.allocator, &context.debugMessenger));
    KDEBUG("Vulkan debugger created.");
#endif

    
    KDEBUG("Creating Vulkan surface...");
    if (!platform_create_vulkan_surface(platformState, &context)) {
        KERROR("Failed to create platform surface!");
        return FALSE;
    }
    KDEBUG("Vulkan surface created.");

    if(!vulkan_device_create(&context)){
        KERROR("Failed to create Vulkan device");
        return FALSE;
    }
    context.swapchains = std::vector<VulkanSwapchain>(context.device.deviceCount);
    context.mainRenderPasses = std::vector<VulkanRenderpass>(context.device.deviceCount);
    context.graphicsCommandBuffers = std::vector<std::vector<VulkanCommandBuffer>>(context.device.deviceCount);
    for(int deviceIndex=0; deviceIndex < context.device.deviceCount; deviceIndex++){
        vulkan_swapchain_create(&context,context.framebufferWidth,context.framebufferHeight,&context.swapchains[deviceIndex],deviceIndex);
         vulkan_renderpass_create(
        &context,
        &context.mainRenderPasses[deviceIndex],
        0, 0, context.framebufferWidth, context.framebufferHeight,
        0.0f, 0.0f, 0.2f, 1.0f,
        1.0f,
        0,deviceIndex);

        create_command_buffers(backend,deviceIndex);


    }


    

    KINFO("Vulkan backend initialized successfully");
    return TRUE;
    

}
void vulkan_renderer_backend_shutdown(RendererBackend* backend){

    // Destroy in Reverse order of create

    for(int deviceIndex=0; deviceIndex < context.device.deviceCount; deviceIndex++){

         for(int i=0; i < context.swapchains[deviceIndex].imageCount; i++){
            if(context.graphicsCommandBuffers[deviceIndex][i].handle != VK_NULL_HANDLE){
                vulkan_command_buffer_free(&context,context.device.graphicsCommandPools[deviceIndex],&context.graphicsCommandBuffers[deviceIndex][i],deviceIndex);
                context.graphicsCommandBuffers[deviceIndex][i].handle = VK_NULL_HANDLE;
                KINFO("Free commandBuffer %i for device %s ",i,context.device.properties[deviceIndex].deviceName);
            }
        }

        vulkan_renderpass_destroy(&context,&context.mainRenderPasses[deviceIndex],deviceIndex);
        vulkan_swapchain_destroy(&context,&context.swapchains[deviceIndex],deviceIndex);
        
    }

    



    vulkan_device_destory(&context);
    
    KINFO("Destroying Vulkan surface");
    vkDestroySurfaceKHR(context.instance,context.surface,context.allocator);

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

i32 find_memory_index(u64 memoryTypeBits,VkMemoryPropertyFlags memoryFlags,int deviceIndex){
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(context.device.physicalDevices[deviceIndex],&memoryProperties);
    for(int i=0; i< memoryProperties.memoryTypeCount; i++){
        if(memoryTypeBits & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & memoryFlags) == memoryFlags){
            return i;
        }
    }
    KWARN("Unable to find compatible memory type");
    return -1;

}

void create_command_buffers(RendererBackend *backend,int deviceIndex){
    context.graphicsCommandBuffers[deviceIndex] = std::vector<VulkanCommandBuffer>(context.swapchains[deviceIndex].imageCount);
        for(int i=0; i < context.swapchains[deviceIndex].imageCount; i++){
            if(context.graphicsCommandBuffers[deviceIndex][i].handle == VK_NULL_HANDLE){
                vulkan_command_buffer_free(&context,context.device.graphicsCommandPools[deviceIndex],&context.graphicsCommandBuffers[deviceIndex][i],deviceIndex);
                vulkan_command_buffer_allocate(&context,context.device.graphicsCommandPools[deviceIndex],TRUE,&context.graphicsCommandBuffers[deviceIndex][i],deviceIndex);
                KINFO("Allocated commandBuffer %i for device %s ",i,context.device.properties[deviceIndex].deviceName);
            }
        }

}