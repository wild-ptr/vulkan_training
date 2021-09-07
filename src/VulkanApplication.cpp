#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <optional>
#include <set>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>

#include "Logger.hpp"
#include "Mesh.hpp"
#include "Shader.hpp"
#include "UniformData.hpp"
#include "Vertex.hpp"
#include "VulkanApplication.hpp"
#include "VulkanFramebuffer.hpp"
#include "Renderable.hpp"
#include "Constants.hpp"


namespace render {

void VulkanApplication::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void VulkanApplication::createSurface()
{
    auto VkErr = glfwCreateWindowSurface(vkInstance.getInstance(), window, nullptr, &surface);

    if (VkErr != VK_SUCCESS) {
        const char* description;
        auto glfwErr = glfwGetError(&description);

        std::cout << "VkError: " << VkErr << std::endl;
        std::cout << "GLFW ERROR: " << glfwErr << " desc: " << description << std::endl;

        throw std::runtime_error("Failed to create surface.");
    }
}

void VulkanApplication::initWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

void VulkanApplication::createGraphicsPipeline()
{
    std::vector<Shader> shaders = {
        Shader { vkDevice->getDevice(), "shaders/vert.spv", EShaderType::VERTEX_SHADER },
        Shader { vkDevice->getDevice(), "shaders/frag.spv", EShaderType::FRAGMENT_SHADER }
    };

    pipeline = std::make_shared<Pipeline>(
        shaders,
        vkSwapchain.getSwapchainExtent(),
        vkDevice->getDevice(),
        vkSwapchainFramebuffer.getRenderPass(),
        Pipeline::vertex_input_tag<Vertex>{});
}

void VulkanApplication::createOffscreenFramebuffer()
{
    std::vector<memory::VulkanImageCreateInfo> attachments;

    memory::VulkanImageCreateInfo inf = {
        .width = 800,
        .height = 600,
        .layerCount = 1,
        .mipLevels = 1,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
    };

    attachments.push_back(inf);

    // just to test new framebuffer module.
    //offscreenFramebuffer = VulkanFramebuffer(vkDevice->getVmaAllocator(), {inf}, 3);
}

void VulkanApplication::createCommandPool()
{
    const auto poolInfo = [this] {
        VkCommandPoolCreateInfo pi {};
        pi.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pi.queueFamilyIndex = vkDevice->getGraphicsQueueIndice();
        pi.flags = 0;
        return pi;
    }();

    if (vkCreateCommandPool(vkDevice->getDevice(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create command pool.");
}

void VulkanApplication::createCommandBuffers()
{
    commandBuffers.resize(vkSwapchainFramebuffer.size());

    const auto allocateInfo = [this] {
        VkCommandBufferAllocateInfo ai {};
        ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ai.commandPool = commandPool;
        // Primary buffer - can be executed directly
        // Secondary - cannot be executed directly, but can be called from Primary buffers.
        ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandBufferCount = vkSwapchainFramebuffer.size();

        return ai;
    }();

    if (vkAllocateCommandBuffers(vkDevice->getDevice(), &allocateInfo, commandBuffers.data()) != VK_SUCCESS)
        throw std::runtime_error("cannot allocate command buffers!");
}

// So this part will need to be a part of main render engine,
// as it has to deal with a loop across all Renderables which will contain all meshes
// and we need to invoke a draw call on every one of those, rebinding vertex buffer offsets
void VulkanApplication::recordCommandBuffers(uint32_t swapchainImgIdx, uint32_t frameInFlightIdx)
{
        // care must be taken not to reset pending buffers!
        vkResetCommandBuffer(commandBuffers[frameInFlightIdx], 0);

        const auto beginInfo = [] {
            VkCommandBufferBeginInfo cbi {};
            cbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cbi.flags = 0;
            cbi.pInheritanceInfo = nullptr;

            return cbi;
        }();

        if (vkBeginCommandBuffer(commandBuffers[frameInFlightIdx], &beginInfo) != VK_SUCCESS)
            throw std::runtime_error("cannot begin command buffer.");

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {0.2f, 0.2f, 0.2f, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};

        const auto renderPassInfo = [swapchainImgIdx, &clearValues, this] {
            VkRenderPassBeginInfo rbi {};
            rbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            rbi.renderPass = vkSwapchainFramebuffer.getRenderPass();
            rbi.framebuffer = vkSwapchainFramebuffer[swapchainImgIdx];

            rbi.renderArea.offset = { 0, 0 };
            rbi.renderArea.extent = vkSwapchain.getSwapchainExtent();

            rbi.clearValueCount = clearValues.size();
            rbi.pClearValues = clearValues.data();

            return rbi;
        }();

        auto cmd = commandBuffers[frameInFlightIdx];
        vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        perFrameData->bind(cmd, frameInFlightIdx);
        to_render_test->cmdBindSetsDrawMeshes(cmd, frameInFlightIdx);

        vkCmdEndRenderPass(cmd);

        if (vkEndCommandBuffer(cmd) != VK_SUCCESS)
            throw std::runtime_error("failed to record command buffer.");
}

void VulkanApplication::initVulkan()
{
#ifdef NDEBUG
    static constexpr bool enableValidationLayers = false;
#else
    static constexpr bool enableValidationLayers = true;
#endif

    vkInstance = VkInstance(enableValidationLayers);
    createSurface();
    vkDevice = std::make_shared<VulkanDevice>(vkInstance.getInstance(), surface);
    vkSwapchain = VulkanSwapchain(*vkDevice, surface, window);
    vkSwapchainFramebuffer = VulkanFramebuffer(vkDevice, vkSwapchain, true);
    createGraphicsPipeline();
    textureManager = std::make_shared<memory::TextureManager>(vkDevice);
    assetLoader = std::make_shared<AssetLoader>(vkDevice, textureManager);
    cameraSystem = std::make_shared<CameraSystem>(window, (float)WIDTH/(float)HEIGHT, 30.0f);
    perFrameData = std::make_shared<memory::PerFrameUniformSystem>(vkDevice, textureManager, cameraSystem, pipeline);
    frameSyncData = std::make_shared<VulkanApplication::FrameSyncData>(vkDevice, vkSwapchain.size());

    // to remove later on
    to_render_test = assetLoader->loadObject("assets/backpack/backpack.obj", pipeline);

    createCommandPool();
    createCommandBuffers();
}

void VulkanApplication::mainLoop()
{
    while (not glfwWindowShouldClose(window)) {
        glfwPollEvents();
        render();
    }
}

void VulkanApplication::render()
{
    cameraSystem->processKeyboardMovement();
    size_t inFlightFrameNo = frameSyncData->getFrameIndex();

    // we need to wait if all frames inflight are used right now.
    vkWaitForFences(vkDevice->getDevice(), 1, &frameSyncData->inFlightFences[inFlightFrameNo], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(vkDevice->getDevice(),
        vkSwapchain.getSwapchain(),
        UINT64_MAX, // timeout in ns, uint64_max disables timeout.
        frameSyncData->imageAvailableSem[inFlightFrameNo],
        VK_NULL_HANDLE, //fence, if applicable.
        &imageIndex);

    recordCommandBuffers(imageIndex, inFlightFrameNo);
    updateUbos(inFlightFrameNo);
    sendBufferToQueue(imageIndex, inFlightFrameNo);

    frameSyncData->advanceFrame();
}

void VulkanApplication::sendBufferToQueue(uint32_t imageIndex, size_t currentFrame)
{
    if (frameSyncData->imagesInFlight[imageIndex] != VK_NULL_HANDLE)
    {
        vkWaitForFences(vkDevice->getDevice(), 1, &frameSyncData->imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }

    frameSyncData->imagesInFlight[imageIndex] = frameSyncData->inFlightFences[currentFrame];

    // The inFlightFences[currentFrame] is lit now.
    // So is imagesInFlight[imageIndex], as they are one and the same fence.

    VkSemaphore waitSemaphores[] = { frameSyncData->imageAvailableSem[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSemaphore signalSemaphores[] = { frameSyncData->renderFinishedSem[currentFrame] };

    const auto submitInfo = [&waitSemaphores, &waitStages, &signalSemaphores, currentFrame, this] {
        VkSubmitInfo submitInfo {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

        // those semaphores will be lit when command buffer execution finishes.
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        return submitInfo;
    }();

    // Reset operation makes fence block! Not the other way around.
    vkResetFences(vkDevice->getDevice(), 1, &frameSyncData->inFlightFences[currentFrame]);

    // dont we have a race condition right there? Yes but this is not multithreaded. (yet!)
    VK_CHECK(vkQueueSubmit(vkDevice->getGraphicsQueue(), 1, &submitInfo, frameSyncData->inFlightFences[currentFrame]));

    VkSwapchainKHR swapchains[] = { vkSwapchain.getSwapchain() };

    const auto presentInfo = [&signalSemaphores, &swapchains, &imageIndex, this] {
        VkPresentInfoKHR pi {};
        pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        pi.waitSemaphoreCount = 1;
        pi.pWaitSemaphores = signalSemaphores;

        pi.swapchainCount = 1;
        pi.pSwapchains = swapchains;
        pi.pImageIndices = &imageIndex;

        return pi;
    }();

    vkQueuePresentKHR(vkDevice->getPresentationQueue(), &presentInfo);
}

void VulkanApplication::updateUbos(size_t frameIdx)
{
    auto model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -1.0f));
    model = glm::scale(model, glm::vec3(1.3f));
    float rads = 0.2 * glfwGetTime();
    model = glm::rotate(model, rads, glm::vec3{0.0, 1.0, 0.0});

    RenderableUbo ubo = {
        .model = model,
        .times = glfwGetTime(),
    };

    to_render_test->updateUniforms(ubo, frameIdx);
    perFrameData->refreshData(frameIdx);
}


void VulkanApplication::cleanup()
{

    vkDestroyCommandPool(vkDevice->getDevice(), commandPool, nullptr);

    //for(auto&& framebuffer : swapChainFramebuffers)
    //    vkDestroyFramebuffer(vkDevice->getDevice(), framebuffer, nullptr);

    //vkDestroyRenderPass(vkDevice->getDevice(), renderPass, nullptr);

    for (auto&& image : vkSwapchain.getSwapchainImageViews())
        vkDestroyImageView(vkDevice->getDevice(), image, nullptr);

    vkDestroySwapchainKHR(vkDevice->getDevice(), vkSwapchain.getSwapchain(), nullptr);
    vkDestroySurfaceKHR(vkInstance.getInstance(), surface, nullptr);
    vkDestroyDevice(vkDevice->getDevice(), nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}

} // namespace render
