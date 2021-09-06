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
void VulkanApplication::recordCommandBuffers()
{
    for (size_t i = 0; i < commandBuffers.size(); ++i) {
        const auto beginInfo = [] {
            VkCommandBufferBeginInfo cbi {};
            cbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cbi.flags = 0;
            cbi.pInheritanceInfo = nullptr;

            return cbi;
        }();

        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
            throw std::runtime_error("cannot begin command buffer.");

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {0.2f, 0.2f, 0.2f, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};

        const auto renderPassInfo = [i, &clearValues, this] {
            VkRenderPassBeginInfo rbi {};
            rbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            rbi.renderPass = vkSwapchainFramebuffer.getRenderPass();
            rbi.framebuffer = vkSwapchainFramebuffer[i];

            rbi.renderArea.offset = { 0, 0 };
            rbi.renderArea.extent = vkSwapchain.getSwapchainExtent();

            rbi.clearValueCount = clearValues.size();
            rbi.pClearValues = clearValues.data();

            return rbi;
        }();

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        perFrameData->bind(commandBuffers[i], 0);
        to_render_test->cmdBindSetsDrawMeshes(commandBuffers[i], 0);

        vkCmdEndRenderPass(commandBuffers[i]);

        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("failed to record command buffer.");
    }
}

void VulkanApplication::createSyncObjects()
{
    imageAvailableSemaphores.resize(consts::maxFramesInFlight);
    renderFinishedSemaphores.resize(consts::maxFramesInFlight);

    inFlightFences.resize(consts::maxFramesInFlight);
    imagesInFlight.resize(vkSwapchainFramebuffer.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < imageAvailableSemaphores.size(); ++i) {
        if (vkCreateSemaphore(vkDevice->getDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i])
                != VK_SUCCESS
            or vkCreateSemaphore(vkDevice->getDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i])
                != VK_SUCCESS
            or vkCreateFence(vkDevice->getDevice(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
            throw std::runtime_error("failed to create semaphores!");
    }
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

    FramebufferAttachmentInfo inf = {
        .ci = {
            .width = 800,
            .height = 600,
            .layerCount = 1,
            .mipLevels = 1,
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT },

        .type = EFramebufferAttachmentType::ATTACHMENT_COLOR
    };

    // just to test new framebuffer module.
    auto framebuf = VulkanFramebuffer(vkDevice, { inf }, 3);

    createGraphicsPipeline();

    textureManager = std::make_shared<memory::TextureManager>(vkDevice);
    assetLoader = std::make_shared<AssetLoader>(vkDevice, textureManager);
    cameraSystem = std::make_shared<CameraSystem>(window, WIDTH/(float)HEIGHT, 100.0f);
    perFrameData = std::make_shared<memory::PerFrameUniformSystem>(vkDevice, textureManager, cameraSystem, pipeline);

    to_render_test = assetLoader->loadObject("assets/backpack/backpack.obj", pipeline);
    perFrameData->refreshData(0);

    createCommandPool();
    createCommandBuffers();
    recordCommandBuffers();

    createSyncObjects();
}

void VulkanApplication::mainLoop()
{
    while (not glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }
}

void VulkanApplication::updateUbos(size_t frameIdx)
{
    auto model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -1.0f));
    model = glm::scale(model, glm::vec3(1.3f));
    float rads = 0.2 * glfwGetTime();
    model = glm::rotate(model, rads, glm::vec3{0.0, 1.0, 0.0});

    auto proj = glm::perspective(glm::radians(45.0f), WIDTH / (float) HEIGHT, 0.1f, 10.0f);
    auto view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    model = proj * view * model;

    RenderableUbo ubo = {
        .model = model,
        .times = glfwGetTime(),
    };

    to_render_test->updateUniforms(ubo, 0);
    perFrameData->refreshData(0);
}

void VulkanApplication::drawFrame()
{
    updateUbos(0);
    // we check whether we can actually start rendering another frame, we want to have
    // a maximum of consts::maxFramesInFlight on queue.
    vkWaitForFences(vkDevice->getDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(vkDevice->getDevice(),
        vkSwapchain.getSwapchain(),
        UINT64_MAX, // timeout in ns, uint64_max disables timeout.
        imageAvailableSemaphores[currentFrame],
        VK_NULL_HANDLE, //fence, if applicable.
        &imageIndex);

    // The first fence is not enough of a synchronization point, as the vkAcquireNextImageKHR
    // can actually give us frames out-of-order, or we can have less images in swapchain
    // than our maximum frames in flight setting. Therefore we also have to check if given
    // swapchain image is not used by another frame, and if so, we have to stall.
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE)
        vkWaitForFences(vkDevice->getDevice(), 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);

    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    // The inFlightFences[currentFrame] is lit now.
    // So is imagesInFlight[imageIndex], as they are one and the same fence.

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };

    const auto submitInfo = [&waitSemaphores, &waitStages, &signalSemaphores, imageIndex, this] {
        VkSubmitInfo submitInfo {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        // This tells us to wait on semaphore, and on what stages we want to wait for which semaphores.
        // waitStages and waitSemaphores indices are linked, so here we will wait on ImageAvailable
        // with pipeline stage that outputs to color attachment.
        // Vertex stage can proceed even before we get an image to draw on in this case.
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

        // those semaphores will be lit when command buffer execution finishes.
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        return submitInfo;
    }();

    // Reset operation makes fence block! Not the other way around.
    vkResetFences(vkDevice->getDevice(), 1, &inFlightFences[currentFrame]);

    // dont we have a race condition right there? Yes but this is not multithreaded. (yet!)
    if (vkQueueSubmit(vkDevice->getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame])
        != VK_SUCCESS)
        throw std::runtime_error("Cannot submit to queue");

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

    currentFrame = (currentFrame + 1) % consts::maxFramesInFlight;
}

void VulkanApplication::cleanup()
{
    for (size_t i = 0; i < imageAvailableSemaphores.size(); ++i) {
        vkDestroySemaphore(vkDevice->getDevice(), imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(vkDevice->getDevice(), renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(vkDevice->getDevice(), inFlightFences[i], nullptr);
    }

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
