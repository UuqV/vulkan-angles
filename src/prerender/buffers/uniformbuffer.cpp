#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <cstdint>
#include <array>
#include "indexbuffer.cpp"

std::vector<VkBuffer> uniformBuffers;
std::vector<VkDeviceMemory> uniformBuffersMemory;
std::vector<void *> uniformBuffersMapped;
VkDeviceSize dynamicAlignment;

struct UniformBufferObject
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

VkDeviceSize queryUBOAlignment()
{
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physicalDevice, &props);
    VkDeviceSize minAlignment = props.limits.minUniformBufferOffsetAlignment;

    std::cout << "minUniformBufferOffsetAlignment: "
              << props.limits.minUniformBufferOffsetAlignment << std::endl;

    VkDeviceSize dynamicAlignment = sizeof(UniformBufferObject);
    if (minAlignment > 0)
    {
        dynamicAlignment = (dynamicAlignment + minAlignment - 1) & ~(minAlignment - 1);
    }
    return dynamicAlignment;
}

void createUniformBuffers()
{
    dynamicAlignment = queryUBOAlignment();
    VkDeviceSize bufferSize = dynamicAlignment * 6;

    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     uniformBuffers[i], uniformBuffersMemory[i]);
    }
}

void cleanupUniformBuffers()
{

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }
}