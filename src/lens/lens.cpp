#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "descriptorset.cpp"

void initLens()
{
    createOffscreenResources();
    createLensRenderPass();
    createLensDescriptorSetLayout();
    createLensPipeline();
    createLensDescriptorSets();
}
