#include "vulkan_test.h"
#include <vector>
static VkInstance instance{};
b8 vulkan_test_init(){

    std::vector<const char*> extensions;
    std::vector<const char*> layers;
    VkInstanceCreateInfo createInfo{};
    extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    extensions.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledExtensionCount = extensions.size();

    KDEBUG("Creating Vulkan Instance");

    if(vkCreateInstance(&createInfo,VK_NULL_HANDLE,&instance) == VK_SUCCESS){
        KDEBUG("Created Vulkan Instance");
        for(int i=0; i < createInfo.enabledExtensionCount; i++){
                KDEBUG("Enabled extensions are >>> %s",createInfo.ppEnabledExtensionNames[i]);

        }
        
        return TRUE;
    }
    else{
        KDEBUG("Failed to Create Vulkan Instance");
        return FALSE;
    }

}

void vulkan_test_shutdown(){
    vkDestroyInstance(instance,VK_NULL_HANDLE);
}