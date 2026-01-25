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
