#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "descriptorset.cpp"

// Lens pass resources

void initLens()
{
    createImageViews();
    createLensRenderPass();
    createLensDescriptorSetLayout();
    createLensPipeline();
    createLensFrameBuffers();
    createLensDescriptorSets();
}

void cleanupLens()
{
    vkDestroyPipeline(device, lensPipeline, nullptr);
    vkDestroyPipelineLayout(device, lensPipelineLayout, nullptr);
    vkDestroyRenderPass(device, lensRenderPass, nullptr);
    vkDestroyDescriptorPool(device, lensDescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, lensDescriptorSetLayout, nullptr);
}