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
        vkDestroyPipeline(device, lensPipeline, nullptr);
        vkDestroyPipelineLayout(device, lensPipelineLayout, nullptr);
        vkDestroyRenderPass(device, lensRenderPass, nullptr);
        vkDestroyDescriptorPool(device, lensDescriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(device, lensDescriptorSetLayout, nullptr);
        vkDestroySampler(device, offscreenSampler, nullptr);
        vkDestroyFramebuffer(device, offscreenFramebuffer, nullptr);
        vkDestroyImageView(device, offscreenImageView, nullptr);
        vkDestroyImage(device, offscreenImage, nullptr);
        vkFreeMemory(device, offscreenImageMemory, nullptr);
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

    void recreateSwapChain()
    {
        int width = 0,
            height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0)
        {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(device);

        cleanupSwapChain();

        createSwapChain();
        createImageViews();
        createFrameBuffers();

        // Recreate offscreen resources at new size
        vkDestroyFramebuffer(device, offscreenFramebuffer, nullptr);
        vkDestroyImageView(device, offscreenImageView, nullptr);
        vkDestroyImage(device, offscreenImage, nullptr);
        vkFreeMemory(device, offscreenImageMemory, nullptr);
        createOffscreenResources();

        // Update descriptor sets with new image view
        createLensDescriptorSets();
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

    void drawFrame()
    {
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
                                                imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
        {
            framebufferResized = false;
            recreateSwapChain();
            return;
        }

        updateUniformBuffer(currentFrame);
        vkResetFences(device, 1, &inFlightFences[currentFrame]);
        vkResetCommandBuffer(commandBuffers[currentFrame], 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(commandBuffers[currentFrame], &beginInfo);

        // === SCENE PASS (renders to offscreen) ===
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = offscreenFramebuffer;
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = {OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT};

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.5f, 0.5f, 0.5f, 1.0f}};
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(OFFSCREEN_WIDTH);
        viewport.height = static_cast<float>(OFFSCREEN_HEIGHT);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = {OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT};
        vkCmdSetScissor(commandBuffers[currentFrame], 0, 1, &scissor);

        VkBuffer vertexBuffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffers[currentFrame], 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffers[currentFrame], indexBuffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);
        vkCmdDrawIndexed(commandBuffers[currentFrame], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffers[currentFrame]);

        // Barrier: ensure offscreen writes complete before fragment shader reads
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = offscreenImage;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
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

        // === LENS PASS (renders to swapchain) ===
        VkRenderPassBeginInfo lensPassInfo{};
        lensPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        lensPassInfo.renderPass = lensRenderPass;
        lensPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
        lensPassInfo.renderArea.offset = {0, 0};
        lensPassInfo.renderArea.extent = swapChainExtent;

        VkClearValue lensClear{};
        lensClear.color = {{1.0f, 0.0f, 1.0f, 1.0f}};
        lensPassInfo.clearValueCount = 1;
        lensPassInfo.pClearValues = &lensClear;

        vkCmdBeginRenderPass(commandBuffers[currentFrame], &lensPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, lensPipeline);

        VkViewport lensViewport{};
        lensViewport.x = 0.0f;
        lensViewport.y = 0.0f;
        lensViewport.width = static_cast<float>(swapChainExtent.width);   // Screen size
        lensViewport.height = static_cast<float>(swapChainExtent.height); // Screen size
        lensViewport.minDepth = 0.0f;
        lensViewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &lensViewport);

        VkRect2D lensScissor{};
        lensScissor.offset = {0, 0};
        lensScissor.extent = swapChainExtent; // Screen size
        vkCmdSetScissor(commandBuffers[currentFrame], 0, 1, &lensScissor);

        // Set viewport and scissor for lens pass too
        vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &lensViewport);
        vkCmdSetScissor(commandBuffers[currentFrame], 0, 1, &lensScissor);

        vkCmdBindDescriptorSets(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                lensPipelineLayout, 0, 1, &lensDescriptorSets[currentFrame], 0, nullptr);
        float lensParams[4] = {
            3.0f, // fov: pi radians = 180 degrees
            1.0f, // strength: 1.0 = full fisheye
            0.5f, // centerX
            0.5f  // centerY
        };
        vkCmdPushConstants(commandBuffers[currentFrame], lensPipelineLayout,
                           VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(lensParams), lensParams);

        vkCmdDraw(commandBuffers[currentFrame], 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffers[currentFrame]);
        vkEndCommandBuffer(commandBuffers[currentFrame]);

        // Submit
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

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        // Present
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        VkSwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            recreateSwapChain();
        }

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