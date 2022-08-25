#include "renderer/vulkan_backend/vulkan_device.h"
#include "core/logger.h"
#include "memory/kmemory.h"
#include <string.h>

typedef struct VulkanPhysicalDeviceRequirements
{
    b8 graphics;
    b8 compute;
    b8 transfer;
    b8 present;
    std::vector<const char *> deviceExtensionNames;
    b8 anisotropySampler;
    b8 discreteGpu;
} VulkanPhysicalDeviceRequirements;

typedef struct VulkanPhysicalDeviceQueueFamilyInfo
{
    u32 graphicsFamilyIndex;
    u32 computeFamilyIndex;
    u32 transferFamilyIndex;
    u32 presentFamilyIndex;
} VulkanPhysicalDeviceQueueFamilyInfo;

b8 physical_device_meets_requirements(VkPhysicalDevice device, VkSurfaceKHR surface, const VkPhysicalDeviceProperties *properties, const VkPhysicalDeviceFeatures *features,
                                      const VulkanPhysicalDeviceRequirements *requirements, VulkanPhysicalDeviceQueueFamilyInfo *queueFamilyInfo, VulkanSwapChainSupportInfo *swapchainSupportInfo,int deviceIndex);

b8 vulkan_create_physical_device_array(VulkanContext *context);

b8 vulkan_device_create(VulkanContext *context)
{
    if (!vulkan_create_physical_device_array(context))
    {
        return false;
    }

    KINFO("Creating Logical devices");
    context->device.logicalDevices = std::vector<VkDevice>(context->device.deviceCount);
    context->device.graphicsQueues = std::vector<VkQueue>(context->device.deviceCount);
    context->device.presentQueues = std::vector<VkQueue>(context->device.deviceCount);
    context->device.transferQueues = std::vector<VkQueue>(context->device.deviceCount);
    context->device.graphicsCommandPools = std::vector<VkCommandPool>(context->device.deviceCount);
    for (int deviceIndex = 0; deviceIndex < context->device.deviceCount; deviceIndex++)
    {

        
        b8 presentSharesGraphicsQueue = context->device.graphicsQueueIndex[deviceIndex] == context->device.presentQueueIndex[deviceIndex];
        b8 transferSharesGraphicsQueue = context->device.graphicsQueueIndex[deviceIndex] == context->device.transferQueueIndex[deviceIndex];
        b8 presentSharesTransferQueue = context->device.presentQueueIndex[deviceIndex] == context->device.transferQueueIndex[deviceIndex];
        u32 indexCount = 0;
        if (!presentSharesGraphicsQueue)
        {
            
            indexCount++;
        }
        if (!transferSharesGraphicsQueue)
        {
            
            indexCount++;
        }
        if (!presentSharesTransferQueue)
        {
            
            indexCount++;
        }
        u32 indices[indexCount];
        u8 index = 0;
        indices[index++] = context->device.graphicsQueueIndex[deviceIndex];
        if (!presentSharesGraphicsQueue)
        {
            indices[index++] = context->device.presentQueueIndex[deviceIndex];
        }
        if (!transferSharesGraphicsQueue)
        {
            indices[index++] = context->device.transferQueueIndex[deviceIndex];
        }
        

        VkDeviceQueueCreateInfo queue_create_infos[indexCount];
        for (u32 i = 0; i < indexCount; ++i)
        {
            queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_infos[i].queueFamilyIndex = indices[i];
            queue_create_infos[i].queueCount = 1;
            queue_create_infos[i].flags = 0;
            queue_create_infos[i].pNext = 0;
            f32 queue_priority = 1.0f;
            queue_create_infos[i].pQueuePriorities = &queue_priority;
           
        }

        // Request device features.
        // TODO: should be config driven
        VkPhysicalDeviceFeatures device_features{};
        device_features.samplerAnisotropy = VK_TRUE; // Request anistrophy

        VkDeviceCreateInfo deviceCreateInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        
        deviceCreateInfo.queueCreateInfoCount = indexCount;
        deviceCreateInfo.pQueueCreateInfos = queue_create_infos;
        deviceCreateInfo.pEnabledFeatures = &device_features;
        deviceCreateInfo.enabledExtensionCount = 1;
        const char *extension_names = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        deviceCreateInfo.ppEnabledExtensionNames = &extension_names;

       
        
        // Create the device.
        VK_CHECK(vkCreateDevice(context->device.physicalDevices[deviceIndex], &deviceCreateInfo,context->allocator,&context->device.logicalDevices[deviceIndex]));
         

        KINFO("Logical device created for %s",context->device.properties[deviceIndex].deviceName);
       

        // Get queues.
        vkGetDeviceQueue(
            context->device.logicalDevices[deviceIndex],
            context->device.graphicsQueueIndex[deviceIndex],
            0,
            &context->device.graphicsQueues[deviceIndex]);

        vkGetDeviceQueue(
            context->device.logicalDevices[deviceIndex],
            context->device.presentQueueIndex[deviceIndex],
            0,
            &context->device.presentQueues[deviceIndex]);

        vkGetDeviceQueue(
            context->device.logicalDevices[deviceIndex],
            context->device.transferQueueIndex[deviceIndex],
            0,
            &context->device.transferQueues[deviceIndex]);
        KINFO("Queues obtained.");
        KINFO("Graphics Queue Index %i",context->device.graphicsQueueIndex[deviceIndex]);
        KINFO("Transfer Queue Index %i",context->device.transferQueueIndex[deviceIndex]);
        KINFO("Present Queue Index %i",context->device.presentQueueIndex[deviceIndex]);

        VkCommandPoolCreateInfo commandPoolCreateInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        commandPoolCreateInfo.queueFamilyIndex = context->device.graphicsQueueIndex[deviceIndex];
        commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        VK_CHECK(vkCreateCommandPool(context->device.logicalDevices[deviceIndex],&commandPoolCreateInfo,context->allocator,&context->device.graphicsCommandPools[deviceIndex]));
        KINFO("Graphics command pool created for %s",context->device.properties[deviceIndex].deviceName);


    }

    return true;
}
void vulkan_device_destory(VulkanContext *context)
{

    
    for (int deviceIndex = 0; deviceIndex < context->device.deviceCount; deviceIndex++)
    {
        KINFO("Destroying Graphics Command Pool for %s",context->device.properties[deviceIndex].deviceName)
        vkDestroyCommandPool(context->device.logicalDevices[deviceIndex],context->device.graphicsCommandPools[deviceIndex],context->allocator);
        KINFO("Destroying Logical device for %s",context->device.properties[deviceIndex].deviceName);
        vkDestroyDevice(context->device.logicalDevices[deviceIndex],context->allocator);
    }
    context->device.logicalDevices.clear();

    KINFO("Releasing Physical devices");
    for (int i = 0; i < context->device.deviceCount; i++)
    {
        context->device.physicalDevices[i] = VK_NULL_HANDLE;
    }
    context->device.physicalDevices.clear();
    context->device.deviceCount = 0;

    if (context->device.swapchainSupport.formats)
    {
        kfree(
            context->device.swapchainSupport.formats,
            sizeof(VkSurfaceFormatKHR) * context->device.swapchainSupport.formatCount,
            MEMORY_TAG_RENDERER);
        context->device.swapchainSupport.formats = 0;
        context->device.swapchainSupport.formatCount = 0;
    }

    if (context->device.swapchainSupport.presentModes)
    {
        kfree(
            context->device.swapchainSupport.presentModes,
            sizeof(VkPresentModeKHR) * context->device.swapchainSupport.presentModeCount,
            MEMORY_TAG_RENDERER);
        context->device.swapchainSupport.presentModes = 0;
        context->device.swapchainSupport.presentModeCount = 0;
    }

    kzero_memory(
        &context->device.swapchainSupport.capabilities,
        sizeof(context->device.swapchainSupport.capabilities));

    context->device.graphicsQueueIndex.clear();
    context->device.presentQueueIndex.clear();
    context->device.transferQueueIndex.clear();
}

void vulkan_device_query_swapchain_support(
    VkPhysicalDevice physicalDevice,
    VkSurfaceKHR surface,
    VulkanSwapChainSupportInfo *swapchainSupportInfo)
{

    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        physicalDevice,
        surface,
        &swapchainSupportInfo->capabilities));

    // Surface formats
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
        physicalDevice,
        surface,
        &swapchainSupportInfo->formatCount,
        0));

    if (swapchainSupportInfo->formatCount != 0)
    {
        if (!swapchainSupportInfo->formats)
        {
            swapchainSupportInfo->formats = (VkSurfaceFormatKHR *)kallocate(sizeof(VkSurfaceFormatKHR) * swapchainSupportInfo->formatCount, MEMORY_TAG_RENDERER);
        }
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice,
            surface,
            &swapchainSupportInfo->formatCount,
            swapchainSupportInfo->formats));
    }

    // Present modes
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
        physicalDevice,
        surface,
        &swapchainSupportInfo->presentModeCount,
        0));
    if (swapchainSupportInfo->presentModeCount != 0)
    {
        if (!swapchainSupportInfo->presentModes)
        {
            swapchainSupportInfo->presentModes = (VkPresentModeKHR *)kallocate(sizeof(VkPresentModeKHR) * swapchainSupportInfo->presentModeCount, MEMORY_TAG_RENDERER);
        }
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice,
            surface,
            &swapchainSupportInfo->presentModeCount,
            swapchainSupportInfo->presentModes));
    }
}

b8 vulkan_device_detect_depth_format(VulkanDevice* device,int deviceIndex){
    const u64 candidate_count = 3;
    VkFormat candidates[3] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT};

    u32 flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    for (u64 i = 0; i < candidate_count; ++i) {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(device->physicalDevices[deviceIndex], candidates[i], &properties);

        if ((properties.linearTilingFeatures & flags) == flags) {
            device->depthFormat = candidates[i];
            return true;
        } else if ((properties.optimalTilingFeatures & flags) == flags) {
            device->depthFormat = candidates[i];
            return true;
        }
    }

    return false;
}

b8 vulkan_create_physical_device_array(VulkanContext *context)
{
    u32 deviceCount = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &deviceCount, VK_NULL_HANDLE));
    if (deviceCount == 0)
    {
        KFATAL("No Vulkan Physical Devices found!");
        return false;
    }
    context->device.physicalDevices = std::vector<VkPhysicalDevice>(deviceCount);
    context->device.deviceNames = std::vector<const char*>(deviceCount);
    context->device.presentQueueIndex = std::vector<u32>(deviceCount);
    context->device.transferQueueIndex = std::vector<u32>(deviceCount);
    context->device.graphicsQueueIndex = std::vector<u32>(deviceCount);
    
    VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &deviceCount, context->device.physicalDevices.data()));
    for (int i = 0; i < deviceCount; i++)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(context->device.physicalDevices[i], &properties);
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(context->device.physicalDevices[i], &features);

        VkPhysicalDeviceMemoryProperties memory;
        vkGetPhysicalDeviceMemoryProperties(context->device.physicalDevices[i], &memory);

        // TODO: These requirements should probably be driven by engine
        // configuration.
        VulkanPhysicalDeviceRequirements requirements{};
        requirements.graphics = true;
        requirements.present = true;
        requirements.transfer = true;
        // NOTE: Enable this if compute will be required.
        // requirements.compute = true;
        requirements.anisotropySampler = true;
        requirements.discreteGpu = true;
        requirements.deviceExtensionNames.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        VulkanPhysicalDeviceQueueFamilyInfo queueFamilyInfo{};

        b8 result = physical_device_meets_requirements(
            context->device.physicalDevices[i],
            context->surface,
            &properties,
            &features,
            &requirements,
            &queueFamilyInfo,
            &context->device.swapchainSupport,i);

        if (result)
        {
            KINFO("Selected device: '%s'.", properties.deviceName);
            // GPU type, etc.
            switch (properties.deviceType)
            {
            default:
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                KINFO("GPU type is Unknown.");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                KINFO("GPU type is Integrated.");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                KINFO("GPU type is Descrete.");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                KINFO("GPU type is Virtual.");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                KINFO("GPU type is CPU.");
                break;
            }
            KINFO(
                "GPU Driver version: %d.%d.%d",
                VK_VERSION_MAJOR(properties.driverVersion),
                VK_VERSION_MINOR(properties.driverVersion),
                VK_VERSION_PATCH(properties.driverVersion));

            // Vulkan API version.
            KINFO(
                "Vulkan API version: %d.%d.%d",
                VK_VERSION_MAJOR(properties.apiVersion),
                VK_VERSION_MINOR(properties.apiVersion),
                VK_VERSION_PATCH(properties.apiVersion));

            // Memory information
            for (u32 j = 0; j < memory.memoryHeapCount; ++j)
            {
                f32 memory_size_gib = (((f32)memory.memoryHeaps[j].size) / 1024.0f / 1024.0f / 1024.0f);
                if (memory.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                {
                    KINFO("Local GPU memory: %.2f GiB", memory_size_gib);
                }
                else
                {
                    KINFO("Shared System memory: %.2f GiB", memory_size_gib);
                }
            }

            
            context->device.graphicsQueueIndex[i] = queueFamilyInfo.graphicsFamilyIndex;
            context->device.presentQueueIndex[i] = queueFamilyInfo.presentFamilyIndex;
            context->device.transferQueueIndex[i] = queueFamilyInfo.transferFamilyIndex;
          
            // NOTE: set compute index here if needed.

            // Keep a copy of properties, features and memory info for later use.
            context->device.properties.push_back(properties);
            context->device.features.push_back(features);
            context->device.memory.push_back(memory);
        }
    }
    // Ensure a device was selected
    if (context->device.physicalDevices.empty())
    {
        KERROR("No physical devices were found which meet the requirements.");
        return false;
    }
    context->device.deviceCount = context->device.physicalDevices.size();

    KINFO("Physical device selected.");
    return true;
}

b8 physical_device_meets_requirements(VkPhysicalDevice device, VkSurfaceKHR surface, const VkPhysicalDeviceProperties *properties, const VkPhysicalDeviceFeatures *features,
                                      const VulkanPhysicalDeviceRequirements *requirements, VulkanPhysicalDeviceQueueFamilyInfo *queueFamilyInfo, VulkanSwapChainSupportInfo *swapchainSupportInfo,int deviceIndex)
{

    queueFamilyInfo->computeFamilyIndex = -1;
    queueFamilyInfo->graphicsFamilyIndex = -1;
    queueFamilyInfo->presentFamilyIndex = -1;
    queueFamilyInfo->transferFamilyIndex = -1;

    // if(requirements->discreteGpu){
    //     if(properties->deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ){
    //         KINFO("Device is not a discrete GPU skipping");
    //         return false;
    //     }
    // }
    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, VK_NULL_HANDLE);
    std::vector<VkQueueFamilyProperties> queueFamilyProperties = std::vector<VkQueueFamilyProperties>(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyProperties.data());

    // Look at each queue and see what queues it supports
    KINFO("Graphics | Present | Compute | Transfer | Name");
    u8 min_transfer_score = 255;
    for (u32 i = 0; i < queueFamilyCount; ++i)
    {
        u8 current_transfer_score = 0;

        // Graphics queue?
        if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            queueFamilyInfo->graphicsFamilyIndex = i;
            ++current_transfer_score;
        }

        // Compute queue?
        if (queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            queueFamilyInfo->computeFamilyIndex = i;
            ++current_transfer_score;
        }

        // Transfer queue?
        if (queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            // Take the index if it is the current lowest. This increases the
            // liklihood that it is a dedicated transfer queue.
            if (current_transfer_score <= min_transfer_score)
            {
                min_transfer_score = current_transfer_score;
                queueFamilyInfo->transferFamilyIndex = i;
            }
        }

        // Present queue?
        VkBool32 supports_present = VK_FALSE;
        VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supports_present));
        if (supports_present)
        {
            queueFamilyInfo->presentFamilyIndex = i;
        }
    }

    // Print out some info about the device
    KINFO("       %d |       %d |       %d |        %d | %s",
          queueFamilyInfo->graphicsFamilyIndex,
          queueFamilyInfo->presentFamilyIndex,
          queueFamilyInfo->computeFamilyIndex,
          queueFamilyInfo->transferFamilyIndex,
          properties->deviceName);

    if (
        (!requirements->graphics || (requirements->graphics && queueFamilyInfo->graphicsFamilyIndex != -1)) &&
        (!requirements->present || (requirements->present && queueFamilyInfo->presentFamilyIndex != -1)) &&
        (!requirements->compute || (requirements->compute && queueFamilyInfo->computeFamilyIndex != -1)) &&
        (!requirements->transfer || (requirements->transfer && queueFamilyInfo->transferFamilyIndex != -1)))
    {
        KINFO("Device meets queue requirements.");
        
        

        vulkan_device_query_swapchain_support(
            device,
            surface,
            swapchainSupportInfo);

        if (swapchainSupportInfo->formatCount < 1 || swapchainSupportInfo->presentModeCount < 1)
        {
            if (swapchainSupportInfo->formats)
            {
                kfree(swapchainSupportInfo->formats, sizeof(VkSurfaceFormatKHR) * swapchainSupportInfo->formatCount, MEMORY_TAG_RENDERER);
            }
            if (swapchainSupportInfo->presentModes)
            {
                kfree(swapchainSupportInfo->presentModes, sizeof(VkPresentModeKHR) * swapchainSupportInfo->presentModeCount, MEMORY_TAG_RENDERER);
            }
            KINFO("Required swapchain support not present, skipping device.");
            return false;
        }
        // Device Extensions
        if (!requirements->deviceExtensionNames.empty())
        {
            u32 availableExtensionCount = 0;
            VkExtensionProperties *availableExtensions = 0;
            VK_CHECK(vkEnumerateDeviceExtensionProperties(device, VK_NULL_HANDLE, &availableExtensionCount, VK_NULL_HANDLE));
            if (availableExtensions != 0)
            {
                availableExtensions = (VkExtensionProperties *)kallocate(sizeof(VkExtensionProperties) * availableExtensionCount, MEMORY_TAG_RENDERER);
                VK_CHECK(vkEnumerateDeviceExtensionProperties(device, VK_NULL_HANDLE, &availableExtensionCount, availableExtensions));

                u32 requiredExtensionCount = requirements->deviceExtensionNames.size();
                for (int i = 0; i < requiredExtensionCount; i++)
                {
                    b8 found = false;
                    for (int j = 0; j < availableExtensionCount; j++)
                    {
                        if (strcmp(requirements->deviceExtensionNames[i], availableExtensions[j].extensionName) == 0)
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        KINFO("Required extension not found: %s skipping device ", requirements->deviceExtensionNames[i]);
                        kfree(availableExtensions, sizeof(VkExtensionProperties) * availableExtensionCount, MEMORY_TAG_RENDERER);
                        return false;
                    }
                }
            }
        }
        // Anisotropy
        if (requirements->anisotropySampler && !features->samplerAnisotropy)
        {
            KINFO("Device does not support Anisotropy");
            return false;
        }
        // Device meets all requirements
        return true;
    }

    return false;
}