#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include "../pipeline.cpp"

void createFrameBuffers()
{
    for (uint32_t i = 0; i < 6; i++)
    {
        VkFramebufferCreateInfo fbInfo{};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = renderPass;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = &cubemapFaceViews[i];
        fbInfo.width = CUBEMAP_SIZE;
        fbInfo.height = CUBEMAP_SIZE;
        fbInfo.layers = 1;

        vkCreateFramebuffer(device, &fbInfo, nullptr, &cubemapFramebuffers[i]);
    }
}
