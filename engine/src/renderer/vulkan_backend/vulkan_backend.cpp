#include "vulkan_backend.h"
#include "vulkan_types.inl"
#include "../../core/logger.h"
#include "../../containers/darray.h"
#include "../../core/application.h"
#include "vulkan_platform.h"
#include "vulkan_device.h"
#include <string.h>
#include "vulkan_swapchain.h"
#include "vulkan_renderpass.h"
#include "vulkan_command_buffer.h"
#include "vulkan_framebuffer.h"
#include "vulkan_fence.h"
#include "vulkan_utils.h"

static VulkanContext context{};
static u64 cachedFramebufferWidth = 0;
static u64 cachedFramebufferHeight = 0;

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT *callbackData,
    void *userData);

i32 find_memory_index(u64 memoryTypeBits, VkMemoryPropertyFlags memoryFlags, int deviceIndex);

void create_command_buffers(RendererBackend *backend, int deviceIndex);
void regenerate_framebuffers(RendererBackend *backend, VulkanSwapchain *swapchain, VulkanRenderpass *renderpass, int deviceIndex);
b8 recreate_swapchain(RendererBackend *backend, int deviceIndex);

b8 vulkan_renderer_backend_initialize(RendererBackend *backend, const char *applicationName, struct PlatformState *platformState)
{
    application_get_framebuffer_size(&cachedFramebufferWidth, &cachedFramebufferHeight);

    // TODO: Custom Allocator
    context.findMemoryIndex = find_memory_index;
    context.allocator = VK_NULL_HANDLE;
    VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.pApplicationName = applicationName;
    appInfo.apiVersion = VK_API_VERSION_1_2;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Kohi Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    std::vector<const char *> extensions = std::vector<const char *>(0);
    std::vector<const char *> requiredLayers = std::vector<const char *>(0);

#ifndef NDEBUG
    extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    extensions.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);
    
    platform_get_required_extension_names(&extensions);
    for (int i = 0; i < extensions.size(); i++)
    {
        KDEBUG("Enabled extensions are >>> %s", extensions[i]);
    }
#ifndef NDEBUG
    KINFO(" Loading Validation layers");
    requiredLayers.emplace_back("VK_LAYER_KHRONOS_validation");
    u32 availableLayerCount = 0;

    VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayerCount, VK_NULL_HANDLE));
    std::vector<VkLayerProperties> availableLayers = std::vector<VkLayerProperties>(availableLayerCount);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers.data()));
    for (int i = 0; i < requiredLayers.size(); i++)
    {
        KINFO("Searching for layer: %s ...", requiredLayers[i]);
        b8 found = FALSE;
        for (int j = 0; j < availableLayerCount; j++)
        {
            KINFO("Available layer: %s", availableLayers[j].layerName);
            if (strcmp(requiredLayers[i], availableLayers[j].layerName) == 0)
            {
                found = TRUE;
                KINFO("Found %s", requiredLayers[i]);
                break;
            }
        }
        if (!found)
        {
            KFATAL("Required validation layer %s is missing", requiredLayers[i]);
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

    VK_CHECK(vkCreateInstance(&createInfo, context.allocator, &context.instance));
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
    if (!platform_create_vulkan_surface(platformState, &context))
    {
        KERROR("Failed to create platform surface!");
        return FALSE;
    }
    KDEBUG("Vulkan surface created.");

    if (!vulkan_device_create(&context))
    {
        KERROR("Failed to create Vulkan device");
        return FALSE;
    }
    context.framebufferWidth = std::vector<u64>(context.device.deviceCount);
    context.framebufferHeight = std::vector<u64>(context.device.deviceCount);
    context.framebufferSizeGeneration = std::vector<u64>(context.device.deviceCount);
    context.framebufferSizeLastGeneration = std::vector<u64>(context.device.deviceCount);
    context.currentDeviceIndex = std::vector<int>(context.device.deviceCount);
    context.lastDeviceIndex = std::vector<int>(context.device.deviceCount);
    context.imageAvalableSemaphores = std::vector<std::vector<VkSemaphore>>(context.device.deviceCount);
    context.queueCompleteSemaphores = std::vector<std::vector<VkSemaphore>>(context.device.deviceCount);
    context.inFlightFences = std::vector<std::vector<VulkanFence>>(context.device.deviceCount);
    context.imagesInFlight = std::vector<std::vector<VulkanFence *>>(context.device.deviceCount);
    context.swapchains = std::vector<VulkanSwapchain>(context.device.deviceCount);
    context.mainRenderPasses = std::vector<VulkanRenderpass>(context.device.deviceCount);
    context.graphicsCommandBuffers = std::vector<std::vector<VulkanCommandBuffer>>(context.device.deviceCount);
    context.currentFrame = std::vector<u32>(context.device.deviceCount);
    context.imageIndex = std::vector<u32>(context.device.deviceCount);
    context.recreatingSwapchain = std::vector<b8>(context.device.deviceCount);
    for (int deviceIndex = 0; deviceIndex < context.device.deviceCount; deviceIndex++)
    {
        context.framebufferWidth[deviceIndex] = cachedFramebufferWidth != 0 ? cachedFramebufferWidth : 1280;
        context.framebufferHeight[deviceIndex] = cachedFramebufferHeight != 0 ? cachedFramebufferHeight : 720;

        vulkan_swapchain_create(&context, context.framebufferWidth[deviceIndex], context.framebufferHeight[deviceIndex], &context.swapchains[deviceIndex], deviceIndex);
        vulkan_renderpass_create(&context, &context.mainRenderPasses[deviceIndex], 0, 0, context.framebufferWidth[deviceIndex], context.framebufferHeight[deviceIndex], 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 0, deviceIndex);

        context.swapchains[deviceIndex].framebuffers = (VulkanFramebuffer *)darray_reserve(VulkanFramebuffer, context.swapchains[deviceIndex].imageCount);
        regenerate_framebuffers(backend, &context.swapchains[deviceIndex], &context.mainRenderPasses[deviceIndex], deviceIndex);
        create_command_buffers(backend, deviceIndex);
        // Vulkan sync objects
        context.imageAvalableSemaphores[deviceIndex] = std::vector<VkSemaphore>(context.swapchains[deviceIndex].maxFramesInFlight);
        context.queueCompleteSemaphores[deviceIndex] = std::vector<VkSemaphore>(context.swapchains[deviceIndex].maxFramesInFlight);
        context.inFlightFences[deviceIndex] = std::vector<VulkanFence>(context.swapchains[deviceIndex].maxFramesInFlight);
        for (int i = 0; i < context.swapchains[deviceIndex].maxFramesInFlight; i++)
        {
            VkSemaphoreCreateInfo semaphoreCreateInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
            vkCreateSemaphore(context.device.logicalDevices[deviceIndex], &semaphoreCreateInfo, context.allocator, &context.imageAvalableSemaphores[deviceIndex][i]);
            vkCreateSemaphore(context.device.logicalDevices[deviceIndex], &semaphoreCreateInfo, context.allocator, &context.queueCompleteSemaphores[deviceIndex][i]);

            // Create the fence in a signaled state, indicating that the first frame has already been "rendered".
            // This will prevent the application from waiting indefinitely for the first frame to render since it
            // cannot be rendered until a frame is "rendered" before it.
            vulkan_fence_create(&context, TRUE, &context.inFlightFences[deviceIndex][i], deviceIndex);
        }
        // In flight fences should not yet exist at this point, so clear the list. These are stored in pointers
        // because the initial state should be 0, and will be 0 when not in use. Acutal fences are not owned
        // by this list.
        context.imagesInFlight[deviceIndex] = std::vector<VulkanFence *>(context.swapchains[deviceIndex].imageCount);
        for (u32 i = 0; i < context.swapchains[deviceIndex].imageCount; ++i)
        {
            context.imagesInFlight[deviceIndex][i] = 0;
        }
    }

    KINFO("Vulkan backend initialized successfully");
    return TRUE;
}
void vulkan_renderer_backend_shutdown(RendererBackend *backend)
{

    // Destroy in Reverse order of create

    for (int deviceIndex = 0; deviceIndex < context.device.deviceCount; deviceIndex++)
    {
        vkDeviceWaitIdle(context.device.logicalDevices[deviceIndex]);

        // destroy sync objects
        for (int i = 0; i < context.swapchains[deviceIndex].maxFramesInFlight; i++)
        {
            if (context.imageAvalableSemaphores[deviceIndex][i] != VK_NULL_HANDLE)
            {

                vkDestroySemaphore(context.device.logicalDevices[deviceIndex], context.imageAvalableSemaphores[deviceIndex][i], context.allocator);
                KINFO("Destroyed imageAvalableSemaphore %i for %s", i, context.device.properties[deviceIndex].deviceName);
            }
            if (context.queueCompleteSemaphores[deviceIndex][i] != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(context.device.logicalDevices[deviceIndex], context.queueCompleteSemaphores[deviceIndex][i], context.allocator);
                KINFO("Destroyed queueCompleteSemaphores %i for %s", i, context.device.properties[deviceIndex].deviceName);
            }
            vulkan_fence_destroy(&context, &context.inFlightFences[deviceIndex][i], deviceIndex);
        }
        context.imageAvalableSemaphores[deviceIndex].clear();
        context.queueCompleteSemaphores[deviceIndex].clear();
        context.inFlightFences[deviceIndex].clear();
        context.imagesInFlight[deviceIndex].clear();

        for (int i = 0; i < context.swapchains[deviceIndex].imageCount; i++)
        {
            if (context.graphicsCommandBuffers[deviceIndex][i].handle != VK_NULL_HANDLE)
            {
                vulkan_command_buffer_free(&context, context.device.graphicsCommandPools[deviceIndex], &context.graphicsCommandBuffers[deviceIndex][i], deviceIndex);
                context.graphicsCommandBuffers[deviceIndex][i].handle = VK_NULL_HANDLE;
                KINFO("Free commandBuffer %i for device %s ", i, context.device.properties[deviceIndex].deviceName);
            }
        }

        for (int i = 0; i < context.swapchains[deviceIndex].imageCount; i++)
        {
            vulkan_framebuffer_destroy(&context, &context.swapchains[deviceIndex].framebuffers[i], deviceIndex);
            KINFO("Destroyed framebuffer %i for device %s ", i, context.device.properties[deviceIndex].deviceName);
        }

        vulkan_renderpass_destroy(&context, &context.mainRenderPasses[deviceIndex], deviceIndex);
        vulkan_swapchain_destroy(&context, &context.swapchains[deviceIndex], deviceIndex);
    }

    vulkan_device_destory(&context);

    KINFO("Destroying Vulkan surface");
    vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);

#ifndef NDEBUG
    if (context.debugMessenger != nullptr)
    {
        KDEBUG("Destroying Vulkan debugger...");
        PFN_vkDestroyDebugUtilsMessengerEXT func =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkDestroyDebugUtilsMessengerEXT");
        func(context.instance, context.debugMessenger, context.allocator);
    }
#endif

    KINFO("Destroying Vulkan instance...");
    vkDestroyInstance(context.instance, context.allocator);
}

void vulkan_renderer_backend_on_resized(RendererBackend *backend, u16 width, u16 height)
{
    int deviceIndex = backend->frameNumber % context.device.deviceCount;
    cachedFramebufferWidth = width;
    cachedFramebufferHeight = height;
    context.framebufferSizeGeneration[deviceIndex]++;
    context.currentDeviceIndex[deviceIndex] = deviceIndex;

    KINFO("Vulkan renderer backend->resized: for %s w/h/gen/deviceIndex: %i/%i/%llu/%i", context.device.properties[deviceIndex].deviceName, width, height, context.framebufferSizeGeneration[deviceIndex],context.currentDeviceIndex[deviceIndex]);
}

b8 vulkan_renderer_backend_begin_frame(RendererBackend *backend, f64 deltaTime)
{

    int deviceIndex = backend->frameNumber % context.device.deviceCount;
    // KDEBUG("Begin frame %i on Device %s",backend->frameNumber,context.device.properties[deviceIndex].deviceName);
    
    if (context.recreatingSwapchain[deviceIndex])
    {
        VkResult result = vkDeviceWaitIdle(context.device.logicalDevices[deviceIndex]);
        if (!vulkan_result_is_success(result))
        {
            KERROR("vulkan_renderer_backend_begin_frame vkDeviceWaitIdle (1) failed: '%s'", vulkan_result_string(result, TRUE));
            return FALSE;
        }
        KINFO("Recreating Swapchain booting");
        return FALSE;
    }
    
    // Check if the framebuffer has been resized. If so, a new swapchain must be created.
    if ((context.swapchainResult == VK_ERROR_OUT_OF_DATE_KHR || context.swapchainResult == VK_SUBOPTIMAL_KHR) || (context.framebufferSizeGeneration[deviceIndex] != context.framebufferSizeLastGeneration[deviceIndex] || context.currentDeviceIndex[deviceIndex] != context.lastDeviceIndex[deviceIndex]))
    {
        
        KDEBUG("CurrentDeviceIndex %i LastDeviceIndex %i framegeneration %i lastframegeneration %i on Device %s",context.currentDeviceIndex[deviceIndex],context.lastDeviceIndex[deviceIndex],context.framebufferSizeGeneration[deviceIndex],context.framebufferSizeLastGeneration[deviceIndex],context.device.properties[deviceIndex].deviceName);

        VkResult result = vkDeviceWaitIdle(context.device.logicalDevices[deviceIndex]);
        if (!vulkan_result_is_success(result))
        {
            KERROR("vulkan_renderer_backend_begin_frame vkDeviceWaitIdle (2) failed: '%s'", vulkan_result_string(result, TRUE));
            return FALSE;
        }
         if (!recreate_swapchain(backend, deviceIndex))
        {
            KERROR("Error while recreating swapchain");
            return FALSE;
        }
        // If the swapchain recreation failed (because, for example, the window was minimized),
        // boot out before unsetting the flag.
       
        // return FALSE;
    }
    // Wait for the execution of the current frame to complete. The fence being free will allow this one to move on.
    if (!vulkan_fence_wait(
            &context,
            &context.inFlightFences[deviceIndex][context.currentFrame[deviceIndex]],
            UINT64_MAX, deviceIndex))
    {
        KWARN("In-flight fence wait failure!");
        return FALSE;
    }

    // Acquire the next image from the swap chain. Pass along the semaphore that should signaled when this completes.
    // This same semaphore will later be waited on by the queue submission to ensure this image is available.
    if (!vulkan_swapchain_acquire_next_image_index(
            &context,
            &context.swapchains[deviceIndex],
            UINT64_MAX,
            context.imageAvalableSemaphores[deviceIndex][context.currentFrame[deviceIndex]],
            0,
            &context.imageIndex[deviceIndex], deviceIndex))
    {
        return FALSE;
    }

    // Begin recording commands.
    VulkanCommandBuffer *commandBuffer = &context.graphicsCommandBuffers[deviceIndex][context.imageIndex[deviceIndex]];
    vulkan_command_buffer_reset(commandBuffer);
    vulkan_command_buffer_begin(commandBuffer, FALSE, FALSE, FALSE);

    // Dynamic state
    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = (f32)context.framebufferHeight[deviceIndex];
    viewport.width = (f32)context.framebufferWidth[deviceIndex];
    viewport.height = -(f32)context.framebufferHeight[deviceIndex];
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Scissor
    VkRect2D scissor;
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width = context.framebufferWidth[deviceIndex];
    scissor.extent.height = context.framebufferHeight[deviceIndex];

    vkCmdSetViewport(commandBuffer->handle, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer->handle, 0, 1, &scissor);

    context.mainRenderPasses[deviceIndex].w = context.framebufferWidth[deviceIndex];
    context.mainRenderPasses[deviceIndex].h = context.framebufferHeight[deviceIndex];

   

    // Begin the render pass.
    // KERROR("Frame Width  %i Frame Height %i Cached WIdth %i Cached Height %i on Device %s",context.framebufferWidth[deviceIndex],context.framebufferHeight[deviceIndex],cachedFramebufferWidth,cachedFramebufferHeight,context.device.properties[deviceIndex].deviceName);
    // vkDeviceWaitIdle(context.device.logicalDevices[deviceIndex]);
    vulkan_renderpass_begin(
        commandBuffer,
        &context.mainRenderPasses[deviceIndex],
        context.swapchains[deviceIndex].framebuffers[context.imageIndex[deviceIndex]].handle);
    return TRUE;
}
b8 vulkan_renderer_backend_end_frame(RendererBackend *backend, f64 deltaTime)
{
    int deviceIndex = backend->frameNumber % context.device.deviceCount;
    // KDEBUG("End frame %i on Device %s",backend->frameNumber,context.device.properties[deviceIndex].deviceName);
    VulkanCommandBuffer *commandBuffer = &context.graphicsCommandBuffers[deviceIndex][context.imageIndex[deviceIndex]];
    vulkan_renderpass_end(commandBuffer, &context.mainRenderPasses[deviceIndex]);
    vulkan_command_buffer_end(commandBuffer);

    // Make sure the previous frame is not using this image (i.e. its fence is being waited on)

    if (context.imagesInFlight[deviceIndex][context.imageIndex[deviceIndex]] != VK_NULL_HANDLE)
    { // was frame
        vulkan_fence_wait(
            &context,
            context.imagesInFlight[deviceIndex][context.imageIndex[deviceIndex]],
            UINT64_MAX, deviceIndex);
    }
    // Mark the image fence as in-use by this frame.
    context.imagesInFlight[deviceIndex][context.currentFrame[deviceIndex]] = &context.inFlightFences[deviceIndex][context.currentFrame[deviceIndex]];

    // Reset the fence for use on the next frame
    vulkan_fence_reset(&context, &context.inFlightFences[deviceIndex][context.currentFrame[deviceIndex]], deviceIndex);

    // Submit the queue and wait for the operation to complete.
    // Begin queue submission
    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};

    // Command buffer(s) to be executed.
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer->handle;

    // The semaphore(s) to be signaled when the queue is complete.
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &context.queueCompleteSemaphores[deviceIndex][context.currentFrame[deviceIndex]];

    // Wait semaphore ensures that the operation cannot begin until the image is available.
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &context.imageAvalableSemaphores[deviceIndex][context.currentFrame[deviceIndex]];

    // Each semaphore waits on the corresponding pipeline stage to complete. 1:1 ratio.
    // VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT prevents subsequent colour attachment
    // writes from executing until the semaphore signals (i.e. one frame is presented at a time)
    VkPipelineStageFlags flags[1] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.pWaitDstStageMask = flags;

    VkResult result = vkQueueSubmit(
        context.device.graphicsQueues[deviceIndex],
        1,
        &submitInfo,
        context.inFlightFences[deviceIndex][context.currentFrame[deviceIndex]].handle);
    if (result != VK_SUCCESS)
    {
        KERROR("vkQueueSubmit failed with result: %s", vulkan_result_string(result, TRUE));
        return FALSE;
    }

    vulkan_command_buffer_update_submitted(commandBuffer);
    // End queue submission

    // Give the image back to the swapchain.
    vulkan_swapchain_present(
        &context,
        &context.swapchains[deviceIndex],
        context.device.graphicsQueues[deviceIndex],
        context.device.presentQueues[deviceIndex],
        context.queueCompleteSemaphores[deviceIndex][context.currentFrame[deviceIndex]],
        context.imageIndex[deviceIndex], deviceIndex);

    return TRUE;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT *callbackData, void *userData)
{
    switch (messageSeverity)
    {
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

i32 find_memory_index(u64 memoryTypeBits, VkMemoryPropertyFlags memoryFlags, int deviceIndex)
{
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(context.device.physicalDevices[deviceIndex], &memoryProperties);
    for (int i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
        if (memoryTypeBits & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & memoryFlags) == memoryFlags)
        {
            return i;
        }
    }
    KWARN("Unable to find compatible memory type");
    return -1;
}

void create_command_buffers(RendererBackend *backend, int deviceIndex)
{
    context.graphicsCommandBuffers[deviceIndex] = std::vector<VulkanCommandBuffer>(context.swapchains[deviceIndex].imageCount);
    for (int i = 0; i < context.swapchains[deviceIndex].imageCount; i++)
    {
        if (context.graphicsCommandBuffers[deviceIndex][i].handle == VK_NULL_HANDLE)
        {
            vulkan_command_buffer_free(&context, context.device.graphicsCommandPools[deviceIndex], &context.graphicsCommandBuffers[deviceIndex][i], deviceIndex);
            vulkan_command_buffer_allocate(&context, context.device.graphicsCommandPools[deviceIndex], TRUE, &context.graphicsCommandBuffers[deviceIndex][i], deviceIndex);
            // KINFO("Allocated commandBuffer %i for device %s ", i, context.device.properties[deviceIndex].deviceName);
        }
    }
}

void regenerate_framebuffers(RendererBackend *backend, VulkanSwapchain *swapchain, VulkanRenderpass *renderpass, int deviceIndex)
{
    KDEBUG("Regenerating framebuffers for %s",context.device.properties[deviceIndex].deviceName);
    for (int i = 0; i < swapchain->imageCount; i++)
    {
        u32 attachmentCount = 2;
        VkImageView attachments[] = {
            swapchain->views[i],
            swapchain->depthAttachment.view};
        vulkan_framebuffer_create(&context, renderpass, context.framebufferWidth[deviceIndex], context.framebufferHeight[deviceIndex], attachmentCount, attachments, &context.swapchains[deviceIndex].framebuffers[i], deviceIndex);
    }
}

b8 recreate_swapchain(RendererBackend *backend, int deviceIndex)
{
    if (context.recreatingSwapchain[deviceIndex])
    {
        KDEBUG("Recreating swapchain already in progress");
        return FALSE;
    }
    if (context.framebufferWidth[deviceIndex] == 0 || context.framebufferHeight[deviceIndex] == 0)
    {
        KDEBUG("Framebuffer too small");
        return FALSE;
    }
    context.recreatingSwapchain[deviceIndex] = TRUE;
    vkDeviceWaitIdle(context.device.logicalDevices[deviceIndex]);
    for (u32 i = 0; i < context.swapchains[deviceIndex].imageCount; ++i) {
        context.imagesInFlight[deviceIndex][i] = 0;
    }

    vulkan_device_query_swapchain_support(context.device.physicalDevices[deviceIndex], context.surface, &context.device.swapchainSupport);
    vulkan_device_detect_depth_format(&context.device, deviceIndex);
    vulkan_swapchain_recreate(&context,cachedFramebufferWidth,cachedFramebufferHeight,&context.swapchains[deviceIndex],deviceIndex);

    // Sync the framebuffer size with the cached sizes.
    context.framebufferWidth[deviceIndex] = cachedFramebufferWidth;
    context.framebufferHeight[deviceIndex] = cachedFramebufferHeight;
    context.mainRenderPasses[deviceIndex].w = context.framebufferWidth[deviceIndex];
    context.mainRenderPasses[deviceIndex].h = context.framebufferHeight[deviceIndex];

    // Update framebuffer size generation.
    context.framebufferSizeLastGeneration[deviceIndex != context.lastDeviceIndex[deviceIndex]? context.lastDeviceIndex[deviceIndex]: deviceIndex] = context.framebufferSizeGeneration[deviceIndex != context.lastDeviceIndex[deviceIndex]? context.lastDeviceIndex[deviceIndex]: deviceIndex];
    context.lastDeviceIndex[deviceIndex] = context.currentDeviceIndex[deviceIndex];

    // cleanup swapchain
    for (u32 i = 0; i < context.swapchains[deviceIndex].imageCount; ++i)
    {
        vulkan_command_buffer_free(&context, context.device.graphicsCommandPools[deviceIndex], &context.graphicsCommandBuffers[deviceIndex][i], deviceIndex);
    }

    // Framebuffers.
    for (u32 i = 0; i < context.swapchains[deviceIndex].imageCount; ++i)
    {
        vulkan_framebuffer_destroy(&context, &context.swapchains[deviceIndex].framebuffers[i], deviceIndex);
    }

    context.mainRenderPasses[deviceIndex].x = 0;
    context.mainRenderPasses[deviceIndex].y = 0;
    context.mainRenderPasses[deviceIndex].w = context.framebufferWidth[deviceIndex];
    context.mainRenderPasses[deviceIndex].h = context.framebufferHeight[deviceIndex];

    regenerate_framebuffers(backend, &context.swapchains[deviceIndex], &context.mainRenderPasses[deviceIndex], deviceIndex);
    vkDeviceWaitIdle(context.device.logicalDevices[deviceIndex]);

    create_command_buffers(backend, deviceIndex);

    // Clear the recreating flag.
    context.recreatingSwapchain[deviceIndex] = FALSE;

    return TRUE;
}