#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "../commands/commandbuffer.cpp"

void initPrerender()
{
    createCubemapResources();
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createFrameBuffers();
    createCommandPool();
    initTexture();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
}
