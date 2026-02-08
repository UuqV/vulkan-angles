#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "../commands/commandbuffer.cpp"

void initPrerender()
{
    createCommandPool();
    createCubemapResources();
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createFrameBuffers();
    initTexture();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
}
