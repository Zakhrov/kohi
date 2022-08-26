#pragma once
#include "../../defines.h"
#include "../../kohi_asserts.h"
#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <vector>






typedef struct VulkanBuffer{
 
    u64 totalSize;
    VkBuffer handle;
    VkBufferUsageFlagBits usage;
    b8 isLocked;
    VkDeviceMemory memory;
    i32 memoryIndex;
    u32 memoryPropertyFlags;



}VulkanBuffer;

typedef struct VulkanSwapChainSupportInfo{
   VkSurfaceCapabilitiesKHR capabilities;
    u32 formatCount;
    VkSurfaceFormatKHR* formats;
    u32 presentModeCount;
    VkPresentModeKHR* presentModes;
}VulkanSwapChainSupportInfo;




typedef struct VulkanDevice{
    
    u32 deviceCount;
    VulkanSwapChainSupportInfo swapchainSupport;


    std::vector<VkPhysicalDevice> physicalDevices;
    std::vector<VkDevice> logicalDevices;
    std::vector<const char*> deviceNames;
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
    int deviceIndex;
  
}VulkanRenderpass;


typedef struct VulkanFramebuffer{
  std::vector<VkImageView> attachments;
  VulkanRenderpass* renderpass;
  u32 attachmentCount;
  VkFramebuffer handle;
}VulkanFramebuffer;


typedef struct VulkanSwapchain{
  VkSurfaceFormatKHR imageFormat;
  u8 maxFramesInFlight;
  VkSwapchainKHR handle;
  u32 imageCount;
  std::vector<VkImage> images;
  std::vector<VkImageView> views;
  VulkanImage depthAttachment;
  std::vector<VulkanFramebuffer> framebuffers;

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
    int deviceIndex;
    int id;

}VulkanCommandBuffer;

typedef struct VulkanFence{
  VkFence handle;
  b8 isSignaled;
}VulkanFence;

typedef struct VulkanShaderStage{

  VkShaderModuleCreateInfo createInfo;
  VkShaderModule handle;
  VkPipelineShaderStageCreateInfo shaderStageCreateInfo;

}VulkanShaderStage;

typedef struct VulkanPipeline{
  VkPipeline handle;
  VkPipelineLayout pipelineLayout;
}VulkanPipeline;

#define OBJECT_SHADER_STAGE_COUNT 2
typedef struct VulkanObjectShader{
  VulkanShaderStage stages[OBJECT_SHADER_STAGE_COUNT];
  VulkanPipeline pipeline;

}VulkanObjectShader;



typedef struct VulkanContext {
    VkInstance instance;
    VkAllocationCallbacks* allocator;
    VulkanDevice device;
    VkSurfaceKHR surface;


    std::vector<u64> framebufferWidth;
    std::vector<u64> framebufferHeight;
    std::vector<int> currentDeviceIndex;
    std::vector<int> lastDeviceIndex;
    std::vector<u64> framebufferSizeGeneration;
    std::vector<u64> framebufferSizeLastGeneration;
    std::vector<VulkanSwapchain> swapchains;
    std::vector<VulkanRenderpass> mainRenderPasses;
    std::vector<std::vector<VulkanCommandBuffer>> graphicsCommandBuffers;
    std::vector<std::vector<VkSemaphore>> imageAvalableSemaphores;
    std::vector<std::vector<VkSemaphore>> queueCompleteSemaphores;
    std::vector<std::vector<VulkanFence>> inFlightFences;
    std::vector<std::vector<VulkanFence*>> imagesInFlight;
    std::vector<u32> imageIndex;
    std::vector<u32> currentFrame;
    std::vector<b8> recreatingSwapchain;
    std::vector<VulkanObjectShader> objectShaders;
    std::vector<VulkanBuffer> vertexBuffers;
    std::vector<VulkanBuffer> indexBuffers;
    std::vector<u32> geometryVertexOffset;
    std::vector<u32> geometryIndexOffset;


    VkResult swapchainResult;
    i32(*findMemoryIndex)(u64 memoryTypeBits,VkMemoryPropertyFlags memoryFlags,int deviceIndex);
#ifndef NDEBUG
  VkDebugUtilsMessengerEXT debugMessenger;
#endif
} VulkanContext;




#define VK_CHECK(expr){ KASSERT(expr == VK_SUCCESS);}


