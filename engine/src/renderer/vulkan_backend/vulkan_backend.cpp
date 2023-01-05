#include "renderer/vulkan_backend/vulkan_backend.h"
#include "renderer/vulkan_backend/vulkan_types.inl"
#include "core/logger.h"
#include "containers/darray.h"
#include "core/application.h"
#include "renderer/vulkan_backend/vulkan_platform.h"
#include "renderer/vulkan_backend/vulkan_device.h"
#include <string.h>
#include "renderer/vulkan_backend/vulkan_swapchain.h"
#include "renderer/vulkan_backend/vulkan_renderpass.h"
#include "renderer/vulkan_backend/vulkan_command_buffer.h"
#include "renderer/vulkan_backend/vulkan_framebuffer.h"
#include "renderer/vulkan_backend/vulkan_fence.h"
#include "renderer/vulkan_backend/vulkan_utils.h"
#include "renderer/vulkan_backend/shaders/vulkan_material_shader.h"
#include "renderer/vulkan_backend/vulkan_buffer.h"
#include "renderer/vulkan_backend/vulkan_image.h"
#include "math/math_types.h"

static VulkanContext context{};
static u64 cachedFramebufferWidth = 0;
static u64 cachedFramebufferHeight = 0;



VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT *callbackData,
    void *userData);

i32 find_memory_index(u64 memoryTypeBits, VkMemoryPropertyFlags memoryFlags, int deviceIndex);

b8 create_buffers(VulkanContext* context,int deviceIndex);

void create_command_buffers(RendererBackend *backend, int deviceIndex);
void regenerate_framebuffers(RendererBackend *backend, VulkanSwapchain *swapchain, VulkanRenderpass *renderpass, int deviceIndex);
b8 recreate_swapchain(RendererBackend *backend, int deviceIndex);

void upload_data_range(VulkanContext* context, VkCommandPool pool, VkFence fence, VkQueue queue, VulkanBuffer* buffer, u64 offset, u64 size, void* data,int deviceIndex) {
    // Create a host-visible staging buffer to upload to. Mark it as the source of the transfer.
    VkMemoryPropertyFlags memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VulkanBuffer staging;
    vulkan_buffer_create(context, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, memoryPropertyFlags, true, &staging,deviceIndex);

    // Load the data into the staging buffer.
    vulkan_buffer_load_data(context, &staging, 0, size, 0, data,deviceIndex);

    // Perform the copy from staging to the device local buffer.
    vulkan_buffer_copy_to(context, pool, fence, queue, staging.handle, 0, buffer->handle, offset, size,deviceIndex);

    // Clean up the staging buffer.
    vulkan_buffer_destroy(context, &staging,deviceIndex);
}

b8 vulkan_renderer_backend_initialize(RendererBackend *backend, const char *applicationName)
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
        b8 found = false;
        for (int j = 0; j < availableLayerCount; j++)
        {
            KINFO("Available layer: %s", availableLayers[j].layerName);
            if (strcmp(requiredLayers[i], availableLayers[j].layerName) == 0)
            {
                found = true;
                KINFO("Found %s", requiredLayers[i]);
                break;
            }
        }
        if (!found)
        {
            KFATAL("Required validation layer %s is missing", requiredLayers[i]);
            return false;
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
    if (!platform_create_vulkan_surface(backend->platformState,&context))
    {
        KERROR("Failed to create platform surface!");
        return false;
    }
    KDEBUG("Vulkan surface created.");

    if (!vulkan_device_create(&context))
    {
        KERROR("Failed to create Vulkan device");
        return false;
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
    context.materialShaders = std::vector<VulkanMaterialShader>(context.device.deviceCount);
    context.vertexBuffers = std::vector<VulkanBuffer>(context.device.deviceCount);
    context.indexBuffers = std::vector<VulkanBuffer>(context.device.deviceCount);
    context.geometryVertexOffset = std::vector<u32>(context.device.deviceCount);
    context.geometryIndexOffset = std::vector<u32>(context.device.deviceCount);
    for (int deviceIndex = 0; deviceIndex < context.device.deviceCount; deviceIndex++)
    {
        context.framebufferWidth[deviceIndex] = cachedFramebufferWidth != 0 ? cachedFramebufferWidth : 640;
        context.framebufferHeight[deviceIndex] = cachedFramebufferHeight != 0 ? cachedFramebufferHeight : 480;
        vulkan_swapchain_create(&context,context.framebufferWidth[deviceIndex],context.framebufferHeight[deviceIndex],&context.swapchains[deviceIndex],deviceIndex);


        
        vulkan_renderpass_create(&context, &context.mainRenderPasses[deviceIndex], 0, 0, context.framebufferWidth[deviceIndex], context.framebufferHeight[deviceIndex], 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 0, deviceIndex);

        context.swapchains[deviceIndex].framebuffers = std::vector<VulkanFramebuffer>(context.swapchains[deviceIndex].imageCount);
        regenerate_framebuffers(backend, &context.swapchains[deviceIndex], &context.mainRenderPasses[deviceIndex], deviceIndex);
        create_command_buffers(backend, deviceIndex);


        

        // Vulkan sync objects
        context.imageAvalableSemaphores[deviceIndex] = std::vector<VkSemaphore>(context.swapchains[deviceIndex].imageCount);
        context.queueCompleteSemaphores[deviceIndex] = std::vector<VkSemaphore>(context.swapchains[deviceIndex].imageCount);
        context.inFlightFences[deviceIndex] = std::vector<VulkanFence>(context.swapchains[deviceIndex].imageCount);
        for (int i = 0; i < context.swapchains[deviceIndex].imageCount; i++)
        {
            VkSemaphoreCreateInfo semaphoreCreateInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
            vkCreateSemaphore(context.device.logicalDevices[deviceIndex], &semaphoreCreateInfo, context.allocator, &context.imageAvalableSemaphores[deviceIndex][i]);
            vkCreateSemaphore(context.device.logicalDevices[deviceIndex], &semaphoreCreateInfo, context.allocator, &context.queueCompleteSemaphores[deviceIndex][i]);

            // Create the fence in a signaled state, indicating that the first frame has already been "rendered".
            // This will prevent the application from waiting indefinitely for the first frame to render since it
            // cannot be rendered until a frame is "rendered" before it.
            vulkan_fence_create(&context, true, &context.inFlightFences[deviceIndex][i], deviceIndex);
        }
        // In flight fences should not yet exist at this point, so clear the list. These are stored in pointers
        // because the initial state should be 0, and will be 0 when not in use. Acutal fences are not owned
        // by this list.
        context.imagesInFlight[deviceIndex] = std::vector<VulkanFence *>(context.swapchains[deviceIndex].imageCount);
        for (u32 i = 0; i < context.swapchains[deviceIndex].imageCount; ++i)
        {
            context.imagesInFlight[deviceIndex][i] = nullptr;
        }

        if (!vulkan_material_shader_create(&context, &context.materialShaders[deviceIndex],deviceIndex)) {
            KERROR("Error loading built-in basic_lighting shader.");
            return false;
        }
        create_buffers(&context,deviceIndex);

    // TODO: temporary test code
    const u32 vert_count = 4;
    Vertex3D verts[vert_count];
    kzero_memory(verts, sizeof(Vertex3D) * vert_count);
    const f32 scaleFactor = 10.0f;

    verts[0].position.x = -0.5 * scaleFactor;
    verts[0].position.y = -0.5 * scaleFactor;
    verts[0].texcoord.x = 0.0f;
    verts[0].texcoord.y = 0.0f;

    verts[1].position.x = 0.5 * scaleFactor;
    verts[1].position.y = 0.5 * scaleFactor;
    verts[1].texcoord.x = 1.0f;
    verts[1].texcoord.y = 1.0f;

    verts[2].position.x = -0.5 * scaleFactor;
    verts[2].position.y = 0.5 * scaleFactor;
    verts[2].texcoord.x = 0.0f;
    verts[2].texcoord.y = 1.0f;

    verts[3].position.x = 0.5 * scaleFactor;
    verts[3].position.y = -0.5 * scaleFactor;
    verts[3].texcoord.x = 1.0f;
    verts[3].texcoord.y = 0.0f;

    const u32 index_count = 6;
    u32 indices[index_count] = {0, 1, 2, 0, 3, 1};

    upload_data_range(&context, context.device.graphicsCommandPools[deviceIndex], 0, context.device.graphicsQueues[deviceIndex], &context.vertexBuffers[deviceIndex], 0, sizeof(Vertex3D) * vert_count, verts,deviceIndex);
    upload_data_range(&context, context.device.graphicsCommandPools[deviceIndex], 0, context.device.graphicsQueues[deviceIndex], &context.indexBuffers[deviceIndex], 0, sizeof(u32) * index_count, indices,deviceIndex);

    
    // TODO: end temp code
    }

    

    KINFO("Vulkan backend initialized successfully");
    return true;
}
void vulkan_renderer_backend_shutdown(RendererBackend *backend)
{

    // Destroy in Reverse order of create

    for (int deviceIndex = 0; deviceIndex < context.device.deviceCount; deviceIndex++)
    {
        vkDeviceWaitIdle(context.device.logicalDevices[deviceIndex]);
        vulkan_buffer_destroy(&context,&context.vertexBuffers[deviceIndex],deviceIndex);
        vulkan_buffer_destroy(&context,&context.indexBuffers[deviceIndex],deviceIndex);
        // destroy shader modules
        vulkan_material_shader_destroy(&context,&context.materialShaders[deviceIndex],deviceIndex);

        // destroy sync objects
        for (int i = 0; i < context.swapchains[deviceIndex].imageCount; i++)
        {
            if (context.imageAvalableSemaphores[deviceIndex][i] != VK_NULL_HANDLE)
            {

                vkDestroySemaphore(context.device.logicalDevices[deviceIndex], context.imageAvalableSemaphores[deviceIndex][i], context.allocator);
                // KINFO("Destroyed imageAvalableSemaphore %i for %s", i, context.device.properties[deviceIndex].deviceName);
            }
            if (context.queueCompleteSemaphores[deviceIndex][i] != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(context.device.logicalDevices[deviceIndex], context.queueCompleteSemaphores[deviceIndex][i], context.allocator);
                // KINFO("Destroyed queueCompleteSemaphores %i for %s", i, context.device.properties[deviceIndex].deviceName);
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
                // KINFO("Free commandBuffer %i for device %s ", i, context.device.properties[deviceIndex].deviceName);
            }
        }

        for (int i = 0; i < context.swapchains[deviceIndex].imageCount; i++)
        {
            vulkan_framebuffer_destroy(&context, &context.swapchains[deviceIndex].framebuffers[i], deviceIndex);
            // KINFO("Destroyed framebuffer %i for device %s ", i, context.device.properties[deviceIndex].deviceName);
        }
        

        vulkan_renderpass_destroy(&context, &context.mainRenderPasses[deviceIndex], deviceIndex);
        vulkan_swapchain_destroy(&context, &context.swapchains[deviceIndex],deviceIndex);
        
    }

    vulkan_device_destory(&context);

    KINFO("Destroying Vulkan surface");
    vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);

#ifndef NDEBUG
    if (context.debugMessenger != VK_NULL_HANDLE)
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
    int deviceIndex = backend->frameNumber %  context.device.deviceCount;
    context.frameDeltaTime = deltaTime;
    // Check if recreating swap chain and boot out.
    if (context.recreatingSwapchain[deviceIndex]) {
        VkResult result = vkDeviceWaitIdle(context.device.logicalDevices[deviceIndex]);
        if (!vulkan_result_is_success(result)) {
            KERROR("vulkan_renderer_backend_begin_frame vkDeviceWaitIdle (1) failed: '%s'", vulkan_result_string(result, true));
            return false;
        }
        KINFO("Recreating swapchain, booting.");
        return false;
    }
      if (context.framebufferSizeGeneration[deviceIndex] != context.framebufferSizeLastGeneration[deviceIndex]) {
        VkResult result = vkDeviceWaitIdle(context.device.logicalDevices[deviceIndex]);
        if (!vulkan_result_is_success(result)) {
            KERROR("vulkan_renderer_backend_begin_frame vkDeviceWaitIdle (2) failed: '%s'", vulkan_result_string(result, true));
            return false;
        }

        // If the swapchain recreation failed (because, for example, the window was minimized),
        // boot out before unsetting the flag.
        if (!recreate_swapchain(backend,deviceIndex)) {
            return false;
        }

        KINFO("Resized, booting.");
        return false;
    }
    // Wait for the execution of the current frame to complete. The fence being free will allow this one to move on.
    if (!vulkan_fence_wait(&context,&context.inFlightFences[deviceIndex][context.currentFrame[deviceIndex]],UINT64_MAX,deviceIndex)) {
        KWARN("In-flight fence wait failure!");
        return false;
    }
    // Acquire the next image from the swap chain. Pass along the semaphore that should signaled when this completes.
    // This same semaphore will later be waited on by the queue submission to ensure this image is available.
    if (!vulkan_swapchain_acquire_next_image_index(
            &context,
            &context.swapchains[deviceIndex],
            UINT64_MAX,
            context.imageAvalableSemaphores[deviceIndex][context.currentFrame[deviceIndex]],
            0,
            &context.imageIndex[deviceIndex],deviceIndex)) {
        return false;
    }
    
    
    // Begin recording commands.
    VulkanCommandBuffer* commandBuffer = &context.graphicsCommandBuffers[deviceIndex][context.imageIndex[deviceIndex]];
    vulkan_command_buffer_reset(commandBuffer);
    vulkan_command_buffer_begin(commandBuffer, false, false, false);
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
    vulkan_renderpass_begin(
        commandBuffer,
        &context.mainRenderPasses[deviceIndex],
        context.swapchains[deviceIndex].framebuffers[context.imageIndex[deviceIndex]].handle);

    
    return true;

    
    
}


void vulkan_renderer_backend_update_global_state(RendererBackend* backend,mat4 projection,mat4 view, vec3 viewPosition,vec4 ambientColor,i32 mode){
    int deviceIndex = backend->frameNumber % context.device.deviceCount;
    
    VkCommandBuffer* commandBuffer = &context.graphicsCommandBuffers[deviceIndex][context.imageIndex[deviceIndex]].handle;
    vulkan_material_shader_use(&context,&context.materialShaders[deviceIndex],deviceIndex);
    context.materialShaders[deviceIndex].globalUBO.projection = projection;
    context.materialShaders[deviceIndex].globalUBO.view = view;
    // TODO: Other UBO properties
    vulkan_material_shader_update_global_state(&context,&context.materialShaders[deviceIndex],context.frameDeltaTime,deviceIndex);

    

   
    
    

}

void vulkan_renderer_backend_update_object(RendererBackend* backend, GeometryRenderData data){
    
    int deviceIndex = backend->frameNumber % context.device.deviceCount;
    vkDeviceWaitIdle(context.device.logicalDevices[deviceIndex]);

    VkCommandBuffer* commandBuffer = &context.graphicsCommandBuffers[deviceIndex][context.imageIndex[deviceIndex]].handle;

    

    vulkan_material_shader_update_object(&context,&context.materialShaders[deviceIndex],data,deviceIndex);

    
     
     // TODO: Temp code
    

    vulkan_material_shader_use(&context,&context.materialShaders[deviceIndex],deviceIndex);

    
     // Bind vertex buffer at offset.
    VkDeviceSize offsets[1] = {0};
    

    vkCmdBindVertexBuffers(*commandBuffer, 0, 1, &context.vertexBuffers[deviceIndex].handle, (VkDeviceSize*)offsets);

    

    // Bind index buffer at offset.
    

    vkCmdBindIndexBuffer(*commandBuffer, context.indexBuffers[deviceIndex].handle, 0, VK_INDEX_TYPE_UINT32);

    

    // Issue the draw.
    

    vkCmdDrawIndexed(*commandBuffer, 6, 1, 0, 0, 0);

    

    // TODO: End temp Code


}

b8 vulkan_renderer_backend_end_frame(RendererBackend *backend, f64 deltaTime)
{
    int deviceIndex = backend->frameNumber % context.device.deviceCount;

    VulkanCommandBuffer* commandBuffer = &context.graphicsCommandBuffers[deviceIndex][context.imageIndex[deviceIndex]];
    vulkan_renderpass_end(commandBuffer,&context.mainRenderPasses[deviceIndex]);
    vulkan_command_buffer_end(commandBuffer);
     // Make sure the previous frame is not using this image (i.e. its fence is being waited on)
    if (context.imagesInFlight[deviceIndex][context.currentFrame[deviceIndex]] != VK_NULL_HANDLE) {  // was frame
        vulkan_fence_wait(
            &context,
            context.imagesInFlight[deviceIndex][context.currentFrame[deviceIndex]],
            UINT64_MAX,deviceIndex);
    }
     // Mark the image fence as in-use by this frame.
    context.imagesInFlight[deviceIndex][context.imageIndex[deviceIndex]] = &context.inFlightFences[deviceIndex][context.currentFrame[deviceIndex]];
    // Reset the fence for use on the next frame
    vulkan_fence_reset(&context, &context.inFlightFences[deviceIndex][context.currentFrame[deviceIndex]],deviceIndex);

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
    if (result != VK_SUCCESS) {
        KERROR("vkQueueSubmit failed with result: %s", vulkan_result_string(result, true));
        return false;
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
        context.imageIndex[deviceIndex],deviceIndex);
    vkDeviceWaitIdle(context.device.logicalDevices[deviceIndex]);

    return true;
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
            vulkan_command_buffer_allocate(&context, context.device.graphicsCommandPools[deviceIndex], true, &context.graphicsCommandBuffers[deviceIndex][i], deviceIndex,i);
            // KINFO("Allocated commandBuffer %i for device %s ", i, context.device.properties[deviceIndex].deviceName);
        }
    }
}

void regenerate_framebuffers(RendererBackend *backend, VulkanSwapchain *swapchain, VulkanRenderpass *renderpass, int deviceIndex)
{

    
    for (int i = 0; i < swapchain->imageCount; i++)
    {
        
        // if(context.swapchains[deviceIndex].framebuffers[i].handle != VK_NULL_HANDLE){
        //     vulkan_framebuffer_destroy(&context,&context.swapchains[deviceIndex].framebuffers[i],deviceIndex);
        // }
        u32 attachmentCount = 2;
        std::vector<VkImageView> attachments = std::vector<VkImageView>(attachmentCount);
        if(swapchain->views[i] != VK_NULL_HANDLE){
            attachments[0] = swapchain->views[i];
        }
        else{
            KFATAL("Invalid swapchain view");
            return;
        }
        if(swapchain->depthAttachment.view != VK_NULL_HANDLE){
            attachments[1] = swapchain->depthAttachment.view;
        }
        else{
            KFATAL("Invalid depth attachment");
            return;
        }
        vulkan_framebuffer_create(&context, renderpass, context.framebufferWidth[deviceIndex], context.framebufferHeight[deviceIndex], attachmentCount, attachments, &context.swapchains[deviceIndex].framebuffers[i], deviceIndex);
    }
}

b8 recreate_swapchain(RendererBackend *backend, int deviceIndex)
{
    if (context.recreatingSwapchain[deviceIndex])
    {
        KDEBUG("Recreating swapchain already in progress");
        return false;
    }
    if (context.framebufferWidth[deviceIndex] == 0 || context.framebufferHeight[deviceIndex] == 0)
    {
        KDEBUG("Framebuffer too small");
        return false;
    }
    context.recreatingSwapchain[deviceIndex] = true;
    vkDeviceWaitIdle(context.device.logicalDevices[deviceIndex]);
    for (u32 i = 0; i < context.swapchains[deviceIndex].imageCount; ++i) {
        context.imagesInFlight[deviceIndex][i] = 0;
    }

    vulkan_device_query_swapchain_support(context.device.physicalDevices[deviceIndex], context.surface, &context.device.swapchainSupport);
    vulkan_device_detect_depth_format(&context.device, deviceIndex);
    

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
    vulkan_renderpass_destroy(&context,&context.mainRenderPasses[deviceIndex],deviceIndex);
    vulkan_renderpass_create(&context, &context.mainRenderPasses[deviceIndex], 0, 0, context.framebufferWidth[deviceIndex], context.framebufferHeight[deviceIndex], 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 0, deviceIndex);
    vulkan_swapchain_recreate(&context,context.framebufferWidth[deviceIndex], context.framebufferHeight[deviceIndex],&context.swapchains[deviceIndex],deviceIndex);

    regenerate_framebuffers(backend, &context.swapchains[deviceIndex], &context.mainRenderPasses[deviceIndex], deviceIndex);
    vkDeviceWaitIdle(context.device.logicalDevices[deviceIndex]);

    create_command_buffers(backend, deviceIndex);

    // Clear the recreating flag.
    context.recreatingSwapchain[deviceIndex] = false;

    return true;
}

b8 create_buffers(VulkanContext* context, int deviceIndex){

    VkMemoryPropertyFlagBits memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        const u64 vertex_buffer_size = sizeof(Vertex3D) * 1024 * 1024;
    if (!vulkan_buffer_create(
            context,
            vertex_buffer_size,
            (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT),
            memory_property_flags,
            true,
            &context->vertexBuffers[deviceIndex],deviceIndex)) {
        KERROR("Error creating vertex buffer.");
        return false;
    }
    context->geometryVertexOffset[deviceIndex] = 0;

    const u64 index_buffer_size = sizeof(u32) * 1024 * 1024;
    if (!vulkan_buffer_create(
            context,
            index_buffer_size,
            (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT),
            memory_property_flags,
            true,
            &context->indexBuffers[deviceIndex],deviceIndex)) {
        KERROR("Error creating vertex buffer.");
        return false;
    }
    context->geometryIndexOffset[deviceIndex] = 0;

    return true;


}

void vulkan_renderer_backend_create_texture_for_device(VulkanBuffer* stagingBuffers,const u8* pixels, struct VulkanTexture* texture, int deviceIndex){
    VulkanTextureData* data = (VulkanTextureData*)kallocate(sizeof(VulkanTextureData),MEMORY_TAG_TEXTURE);
    texture->textureData[deviceIndex] = data;


    
    


    
    VkDeviceSize imageSize = texture->width * texture->height * texture->channelCount;

    // NOTE: Assumes 8 bits per channel
    VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM;

    
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    
    vulkan_buffer_create(&context,imageSize,usage,memoryPropertyFlags,true,stagingBuffers,deviceIndex);
    vulkan_buffer_load_data(&context,stagingBuffers,0,imageSize,0,pixels,deviceIndex);
    
    // NOTE: Lots of assumptions here
    
    vulkan_image_create(
        &context,
        VK_IMAGE_TYPE_2D,
        texture->width,
        texture->height,
        imageFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        true,
        VK_IMAGE_ASPECT_COLOR_BIT,
        &data->image,deviceIndex);
    
    VulkanCommandBuffer tempBuffer;
    VkCommandPool pool = context.device.graphicsCommandPools[deviceIndex];
    VkQueue queue = context.device.graphicsQueues[deviceIndex];
    
    vulkan_command_buffer_allocate_and_begin_single_use(&context, pool, &tempBuffer,deviceIndex,tempBuffer.id);
    

    // Transition the layout from whatever it is currently to optimal for recieving data.
    
    vulkan_image_transition_layout(
        &context,
        &tempBuffer,
        &data->image,
        imageFormat,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,deviceIndex);
    

    // Copy the data from the buffer.
    
    vulkan_image_copy_from_buffer(&context, &data->image, stagingBuffers->handle, &tempBuffer,deviceIndex);
    
    // vkDeviceWaitIdle(context.device.logicalDevices[deviceIndex]);
    

    // Transition from optimal for data reciept to shader-read-only optimal layout.
    vulkan_image_transition_layout(
        &context,
        &tempBuffer,
        &data->image,
        imageFormat,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,deviceIndex);
    
    vulkan_command_buffer_end_single_use(&context, pool, &tempBuffer, queue,deviceIndex);
    

    // Create a sampler for the texture
    VkSamplerCreateInfo samplerInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    // TODO: These filters should be configurable.
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    VkResult result = vkCreateSampler(context.device.logicalDevices[deviceIndex], &samplerInfo, context.allocator, &data->sampler);
    if (!vulkan_result_is_success(VK_SUCCESS)) {
        KERROR("Error creating texture sampler: %s", vulkan_result_string(result, true));
        return;
    }
}
void vulkan_renderer_backend_create_texture(RendererBackend* backend, const u8* pixels, Texture* texture){
    int deviceIndex = 0;
    

    // Internal Data creation
    // TODO: Use an allocator for this
    
    VulkanTexture* vulkanTexture = (VulkanTexture*)kallocate(sizeof(VulkanTexture),MEMORY_TAG_TEXTURE);
    vulkanTexture->textureData = std::vector<VulkanTextureData*>(context.device.deviceCount);
    vulkanTexture->width = texture->width;
    vulkanTexture->height = texture->height;
    vulkanTexture->channelCount = texture->channelCount;
    vulkanTexture->id = texture->id;

    
    
    
    std::vector<VulkanBuffer> stagingBuffers = std::vector<VulkanBuffer>(context.device.deviceCount);
    
    for(deviceIndex = 0; deviceIndex < context.device.deviceCount; deviceIndex++){
        
            vulkan_renderer_backend_create_texture_for_device(&stagingBuffers[deviceIndex], pixels, vulkanTexture, deviceIndex);
            vulkan_buffer_destroy(&context, &stagingBuffers[deviceIndex], deviceIndex);
            
        }
    
    texture->internalData = vulkanTexture;
    texture->hasTransparency = vulkanTexture->hasTransparency;
    texture->generation++;
    

        
}

void vulkan_renderer_backend_destroy_texture_for_device(VulkanTextureData* data,int deviceIndex){
    KINFO("Destroying Texture for %s ",context.device.properties[deviceIndex].deviceName);
    
    
    
    vkDeviceWaitIdle(context.device.logicalDevices[deviceIndex]);

    vulkan_image_destroy(&context, &data->image,deviceIndex);
    kzero_memory(&data->image, sizeof(VulkanImage));
    vkDestroySampler(context.device.logicalDevices[deviceIndex], data->sampler, context.allocator);
    kzero_memory(&data->sampler,sizeof(VkSampler));
    KINFO("Destroyed Texture for %s ",context.device.properties[deviceIndex].deviceName);

}

void vulkan_renderer_backend_destroy_texture(RendererBackend* backend, Texture* texture){
    KINFO("Destroying Texture %d in Vulkan Backend",texture->id);
    int deviceIndex = 0;
    VulkanTexture* vulkanTexture = (VulkanTexture*)texture->internalData;
    
    if(vulkanTexture){
        for( deviceIndex = 0; deviceIndex < context.device.deviceCount; deviceIndex++){
            VulkanTextureData* data = vulkanTexture->textureData[deviceIndex];
            if(data){
                
                vulkan_renderer_backend_destroy_texture_for_device(data,deviceIndex);
                kfree(vulkanTexture->textureData[deviceIndex], sizeof(VulkanTextureData), MEMORY_TAG_TEXTURE);
            }
            
        }
        
    }
    kzero_memory(texture, sizeof(Texture));
    

}

b8 vulkan_renderer_backend_create_material(Material* material){
    
    int deviceIndex = 0;
    if(material){

        for(deviceIndex = 0; deviceIndex < context.device.deviceCount; deviceIndex++){

            if(!vulkan_material_shader_acquire_resources(&context,&context.materialShaders[deviceIndex],material,deviceIndex)){
            KERROR("Could not acquire resource for Material");
            return false;

            }
            KDEBUG("VULKAN BACKEND MATERIAL STUB %d Device %s",material->diffuseMap.texture->internalData,context.device.properties[deviceIndex].deviceName);
        }
        return true;
    }
    return false;

}
void vulkan_renderer_backend_destroy_material(Material* material){
    
    for(int deviceIndex = 0; deviceIndex < context.device.deviceCount; deviceIndex++){
        vulkan_material_shader_release_resources(&context,&context.materialShaders[deviceIndex],material,deviceIndex);
        KDEBUG("VULKAN BACKEND MATERIAL STUB DESTROY %s",context.device.properties[deviceIndex].deviceName);
    }
    material->internalId = INVALID_ID;
}