#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include "../commands/commandpool.cpp"

const uint32_t OFFSCREEN_WIDTH = WIDTH * 2.0;
const uint32_t OFFSCREEN_HEIGHT = HEIGHT * 2.0;

// Cubemap resources
VkImage cubemapImage;
VkDeviceMemory cubemapImageMemory;
VkImageView cubemapImageView;
VkSampler cubemapSampler;
VkFramebuffer cubemapFramebuffers[6];
VkImageView cubemapFaceViews[6];

const uint32_t CUBEMAP_SIZE = 1024; // Each face resolution

void createCubemapResources()
{

    // Create cubemap image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = {CUBEMAP_SIZE, CUBEMAP_SIZE, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 6;
    imageInfo.format = swapChainImageFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    vkCreateImage(device, &imageInfo, nullptr, &cubemapImage);

    // Allocate memory
    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device, cubemapImage, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vkAllocateMemory(device, &allocInfo, nullptr, &cubemapImageMemory);
    vkBindImageMemory(device, cubemapImage, cubemapImageMemory, 0);

    // Create cubemap image view (for sampling all 6 faces)
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = cubemapImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.format = swapChainImageFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 6;

    vkCreateImageView(device, &viewInfo, nullptr, &cubemapImageView);

    // Create individual face views (for rendering to each face)
    for (uint32_t i = 0; i < 6; i++)
    {
        VkImageViewCreateInfo faceViewInfo{};
        faceViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        faceViewInfo.image = cubemapImage;
        faceViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        faceViewInfo.format = swapChainImageFormat;
        faceViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        faceViewInfo.subresourceRange.baseMipLevel = 0;
        faceViewInfo.subresourceRange.levelCount = 1;
        faceViewInfo.subresourceRange.baseArrayLayer = i;
        faceViewInfo.subresourceRange.layerCount = 1;

        vkCreateImageView(device, &faceViewInfo, nullptr, &cubemapFaceViews[i]);
    }

    // Create sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16.0f;

    vkCreateSampler(device, &samplerInfo, nullptr, &cubemapSampler);

    VkCommandBuffer cmd = beginSingleTimeCommands();

    VkImageMemoryBarrier initBarrier{};
    initBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    initBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    initBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    initBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    initBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    initBarrier.image = cubemapImage;
    initBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    initBarrier.subresourceRange.baseMipLevel = 0;
    initBarrier.subresourceRange.levelCount = 1;
    initBarrier.subresourceRange.baseArrayLayer = 0;
    initBarrier.subresourceRange.layerCount = 6;
    initBarrier.srcAccessMask = 0;
    initBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &initBarrier);

    endSingleTimeCommands(cmd);
}

void cleanupCubemapResources()
{
    vkDestroySampler(device, cubemapSampler, nullptr);
    vkDestroyImageView(device, cubemapImageView, nullptr);
    for (int i = 0; i < 6; i++)
    {
        vkDestroyImageView(device, cubemapFaceViews[i], nullptr);
    }
    vkDestroyImage(device, cubemapImage, nullptr);
    vkFreeMemory(device, cubemapImageMemory, nullptr);
}