#include <chrono>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <fstream>
#include <array>
#include "lens/lens.cpp"
#include "sync.cpp"

class HelloTriangleApplication
{
public:
    void run()
    {
        initWindow(this);
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    void cleanup()
    {
        vkDeviceWaitIdle(device);
        cleanupLens();
        cleanupCubemapResources();
        cleanupSwapChain();
        cleanupTexture();
        cleanupUniformBuffers();
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        cleanupIndexBuffer();
        cleanupVertexBuffer();
        cleanupSyncObjects();
        vkDestroyCommandPool(device, commandPool, nullptr);
        cleanupPipeline();
        vkDestroyRenderPass(device, renderPass, nullptr);
        cleanupEnvironment();
    }

    void initVulkan()
    {
        if (enableValidationLayers && !checkValidationLayerSupport())
        {
            throw std::runtime_error("validationlayers requested, but not available!");
        }
        initEnvironment();
        createLogicalDevice();
        createSwapChain();
        initPrerender();
        initLens();
        createSyncObjects();
    }

    void updateUniformBuffer(uint32_t currentImage)
    {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(1.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(120.0f), OFFSCREEN_WIDTH / (float)OFFSCREEN_HEIGHT, 0.01f, 10.0f);
        ubo.proj[1][1] *= -1;

        memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }

    bool checkValidationLayerSupport()
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char *layerName : validationLayers)
        {
            bool layerFound = false;
            for (const auto &layerProperties : availableLayers)
            {
                if (strcmp(layerName, layerProperties.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound)
            {
                return false;
            }
        }

        return true;
    }
    glm::mat4 getCubemapViewMatrix(uint32_t face, glm::vec3 pos)
    {
        switch (face)
        {
        case 0:
            return glm::lookAt(pos, pos + glm::vec3(1, 0, 0), glm::vec3(0, -1, 0)); // +X
        case 1:
            return glm::lookAt(pos, pos + glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0)); // -X
        case 2:
            return glm::lookAt(pos, pos + glm::vec3(0, -1, 0), glm::vec3(0, 0, -1)); // +Y
        case 3:
            return glm::lookAt(pos, pos + glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)); // -Y
        case 4:
            return glm::lookAt(pos, pos + glm::vec3(0, 0, 1), glm::vec3(0, -1, 0)); // +Z
        case 5:
            return glm::lookAt(pos, pos + glm::vec3(0, 0, -1), glm::vec3(0, -1, 0)); // -Z
        }
        return glm::mat4(1.0f);
    }
    void drawFrame()
    {
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
                                                imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
        {
            framebufferResized = false;
            // recreateSwapChain();
            return;
        }

        vkResetFences(device, 1, &inFlightFences[currentFrame]);
        vkResetCommandBuffer(commandBuffers[currentFrame], 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(commandBuffers[currentFrame], &beginInfo);

        // Camera position
        glm::vec3 camPos = glm::vec3(0.0f, -0.5f, 2.7f);

        // 90° FOV projection for cubemap faces
        glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
        proj[1][1] *= -1;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(CUBEMAP_SIZE);
        viewport.height = static_cast<float>(CUBEMAP_SIZE);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = {CUBEMAP_SIZE, CUBEMAP_SIZE};

        VkClearValue clearValue{};
        clearValue.color = {{0.1f, 0.1f, 0.1f, 1.0f}};
        // Map the entire buffer once before the loop
        char *mappedData;
        vkMapMemory(device, uniformBuffersMemory[currentFrame], 0,
                    dynamicAlignment * 6, 0, (void **)&mappedData);

        // Write all 6 UBOs upfront
        for (uint32_t face = 0; face < 6; face++)
        {
            UniformBufferObject ubo{};

            static auto startTime = std::chrono::high_resolution_clock::now();
            auto currentTime = std::chrono::high_resolution_clock::now();
            float time = std::chrono::duration<float, std::chrono::seconds::period>(
                             currentTime - startTime)
                             .count();

            ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f),
                                    glm::vec3(0.0f, 0.0f, 1.0f));
            ubo.view = getCubemapViewMatrix(face, camPos);
            ubo.proj = proj;

            memcpy(mappedData + face * dynamicAlignment, &ubo, sizeof(ubo));
        }

        vkUnmapMemory(device, uniformBuffersMemory[currentFrame]);

        // Draw loop — bind with dynamic offset per face
        for (uint32_t face = 0; face < 6; face++)
        {
            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = renderPass;
            renderPassInfo.framebuffer = cubemapFramebuffers[face];
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = {CUBEMAP_SIZE, CUBEMAP_SIZE};
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearValue;
            vkCmdBeginRenderPass(commandBuffers[currentFrame], &renderPassInfo,
                                 VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(commandBuffers[currentFrame],
                              VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
            vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &viewport);
            vkCmdSetScissor(commandBuffers[currentFrame], 0, 1, &scissor);

            // KEY CHANGE: pass dynamic offset
            uint32_t dynamicOffset = static_cast<uint32_t>(face * dynamicAlignment);
            vkCmdBindDescriptorSets(commandBuffers[currentFrame],
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    pipelineLayout, 0, 1,
                                    &descriptorSets[currentFrame],
                                    1, &dynamicOffset); // <-- was 0, nullptr

            VkBuffer vertexBuffers[] = {vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffers[currentFrame], 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffers[currentFrame], indexBuffer, 0,
                                 VK_INDEX_TYPE_UINT16);
            vkCmdDrawIndexed(commandBuffers[currentFrame],
                             static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

            vkCmdEndRenderPass(commandBuffers[currentFrame]);
        }

        // Barrier: transition cubemap to shader read
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = cubemapImage;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 6;
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            commandBuffers[currentFrame],
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        // === LENS PASS ===
        VkRenderPassBeginInfo lensPassInfo{};
        lensPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        lensPassInfo.renderPass = lensRenderPass;
        lensPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
        lensPassInfo.renderArea.offset = {0, 0};
        lensPassInfo.renderArea.extent = swapChainExtent;

        VkClearValue lensClear{};
        lensClear.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        lensPassInfo.clearValueCount = 1;
        lensPassInfo.pClearValues = &lensClear;

        vkCmdBeginRenderPass(commandBuffers[currentFrame], &lensPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, lensPipeline);

        VkViewport lensViewport{};
        lensViewport.width = static_cast<float>(swapChainExtent.width);
        lensViewport.height = static_cast<float>(swapChainExtent.height);
        lensViewport.minDepth = 0.0f;
        lensViewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &lensViewport);

        VkRect2D lensScissor{};
        lensScissor.extent = swapChainExtent;
        vkCmdSetScissor(commandBuffers[currentFrame], 0, 1, &lensScissor);

        vkCmdBindDescriptorSets(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                lensPipelineLayout, 0, 1, &lensDescriptorSets[currentFrame], 0, nullptr);

        float lensParams[4] = {3.14159f, 0.0f, 0.5f, 0.5f}; // FOV in radians, unused, centerX, centerY
        vkCmdPushConstants(commandBuffers[currentFrame], lensPipelineLayout,
                           VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(lensParams), lensParams);

        vkCmdDraw(commandBuffers[currentFrame], 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffers[currentFrame]);
        vkEndCommandBuffer(commandBuffers[currentFrame]);

        // Submit and present (same as before)
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        VkSwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        vkQueuePresentKHR(presentQueue, &presentInfo);

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }
    void mainLoop()
    {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            drawFrame();
        }
    }
};

int main()
{
    HelloTriangleApplication app;

    try
    {
        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}