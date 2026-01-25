#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include "../pipeline.cpp"

void createFrameBuffers()
{

    // Framebuffer for your existing pass (with depth if you have it)
    std::array<VkImageView, 2> attachments = {offscreenImageView};

    VkFramebufferCreateInfo fbInfo{};
    fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.renderPass = renderPass; // Your existing render pass
    fbInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    fbInfo.pAttachments = attachments.data();
    fbInfo.width = swapChainExtent.width;
    fbInfo.height = swapChainExtent.height;
    fbInfo.layers = 1;

    vkCreateFramebuffer(device, &fbInfo, nullptr, &offscreenFramebuffer);
}
