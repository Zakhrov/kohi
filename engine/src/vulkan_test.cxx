#include "vulkan_test.h"
static VkInstance instance{};
b8 vulkan_test_init(){
    
    VkInstanceCreateInfo createInfo{};

    KDEBUG("Creating Vulkan Instance");

    if(vkCreateInstance(&createInfo,VK_NULL_HANDLE,&instance) == VK_SUCCESS){
        KDEBUG("Created Vulkan Instance");
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