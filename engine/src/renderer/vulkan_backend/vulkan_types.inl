#pragma once

#include "../../defines.h"
#include "../../kohi_asserts.h"

#include <vulkan/vulkan.h>
#include <vector>


typedef struct VulkanSwapChainSupportInfo{
   VkSurfaceCapabilitiesKHR capabilities;
    u32 formatCount;
    VkSurfaceFormatKHR* formats;
    u32 presentModeCount;
    VkPresentModeKHR* presentModes;
}VulkanSwapChainSupportInfo;


typedef struct VulkanDevice{
    std::vector<VkPhysicalDevice> physicalDevices;
    std::vector<VkDevice> logicalDevices;
    std::vector<const char*> deviceNames;
    u32 deviceCount;
    VulkanSwapChainSupportInfo swapchainSupport;
    std::vector<u32> graphicsQueueIndex;
    std::vector<u32> presentQueueIndex;
    std::vector<u32> transferQueueIndex;
    std::vector<VkQueue> graphicsQueues;
    std::vector<VkQueue> transferQueues;
    std::vector<VkQueue> presentQueues;
    std::vector<VkCommandPool> graphicsCommandPools;
    std::vector<VkPhysicalDeviceProperties> properties;
    std::vector<VkPhysicalDeviceFeatures> features;
    std::vector<VkPhysicalDeviceMemoryProperties> memory;
    VkFormat depthFormat;
}VulkanDevice;


typedef struct VulkanImage{
  VkImage handle;
  VkDeviceMemory memory;
  VkImageView view;
  u32 width;
  u32 height;
}VulkanImage;

typedef enum VulkanRenderpassState{
    READY,
    RECORDING,
    IN_RENDER_PASS,
    RECORDING_ENDED,
    SUBMITTED,
    NOT_ALLOCATED
}VulkanRenderpassState;
typedef struct VulkanRenderpass{
   VkRenderPass handle;
    f32 x, y, w, h;
    f32 r, g, b, a;

    f32 depth;
    u32 stencil;

    VulkanRenderpassState state;
  
}VulkanRenderpass;


typedef struct VulkanSwapchain{
  VkSurfaceFormatKHR imageFormat;
  u8 maxFramesInFlight;
  VkSwapchainKHR handle;
  u32 imageCount;
  VkImage* images;
  VkImageView* views;
  VulkanImage depthAttachment;

}VulkanSwapchain;

typedef enum VulkanCommandBufferState{
  COMMAND_BUFFER_STATE_READY,
  COMMAND_BUFFER_STATE_RECORDING,
  COMMAND_BUFFER_STATE_IN_RENDER_PASS,
  COMMAND_BUFFER_STATE_RECORDING_ENDED,
  COMMAND_BUFFER_STATE_SUBMITTED,
  COMMAND_BUFFER_STATE_NOT_ALLOCATED
}VulkanCommandBufferState;

typedef struct VulkanCommandBuffer{

    VkCommandBuffer handle;

    // Command buffer state.
    VulkanCommandBufferState state;

}VulkanCommandBuffer;




typedef struct VulkanContext {
    VkInstance instance;
    VkAllocationCallbacks* allocator;
    VulkanDevice device;
    VkSurfaceKHR surface;
    u64 framebufferWidth;
    u64 framebufferHeight;
    std::vector<VulkanSwapchain> swapchains;
    std::vector<VulkanRenderpass> mainRenderPasses;
    std::vector<std::vector<VulkanCommandBuffer>> graphicsCommandBuffers;
    u32 imageIndex;
    u32 currentFrame;
    b8 recreatingSwapchain;
    i32(*findMemoryIndex)(u64 memoryTypeBits,VkMemoryPropertyFlags memoryFlags,int deviceIndex);
#ifndef NDEBUG
  VkDebugUtilsMessengerEXT debugMessenger;
#endif
} VulkanContext;

#define VK_CHECK(expr){ KASSERT(expr == VK_SUCCESS);}