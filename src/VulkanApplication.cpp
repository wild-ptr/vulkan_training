#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>
#include <set>
#include <cstring>
#include <iostream>
#include <optional>
#include <cstdint>
#include <algorithm>

#include "VulkanApplication.hpp"
#include "Logger.hpp"
#include "Shader.hpp"
#include "Vertex.hpp"
#include "TriangleMesh.hpp"
#include "UniformData.hpp"
#include "VulkanFramebuffer.hpp"

namespace
{

static constexpr auto maxFramesInFlight = 2u;


// this will be deleted in the future, dw.
render::Mesh loadTriangleAsMesh(VmaAllocator allocator)
{
    std::vector<render::Vertex> mesh_data;
    mesh_data.resize(3);

    mesh_data[0].pos = { 1.f, 1.f, 0.0f };
	mesh_data[1].pos = {-1.f, 1.f, 0.0f };
	mesh_data[2].pos = { 0.f,-1.f, 0.0f };

// we will use this as color data for now.
    mesh_data[0].surf_normals = { 1.f, 0.f, 0.0f };
	mesh_data[1].surf_normals = { 0.f, 1.f, 0.0f };
	mesh_data[2].surf_normals = { 0.f, 0.f, 1.0f };

    return render::Mesh(mesh_data, allocator);
}

} // anonymous namespace

namespace render
{

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

    if (VkErr != VK_SUCCESS)
    {
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
        Shader{vkDevice.getDevice(), "shaders/vert.spv", EShaderType::VERTEX_SHADER},
        Shader{vkDevice.getDevice(), "shaders/frag.spv", EShaderType::FRAGMENT_SHADER}
    };

    pipeline = Pipeline(shaders, vkSwapchain.getSwapchainExtent(), vkDevice.getDevice(),
            vkSwapchainFramebuffer.getRenderPass(),
            Pipeline::vertex_input_tag<Vertex>{});
}

void VulkanApplication::createOffscreenFramebuffer()
{
    std::vector<memory::VulkanImageCreateInfo> attachments;

    memory::VulkanImageCreateInfo inf =
    {
        .width = 800,
        .height = 600,
        .layerCount = 1,
        .mipLevels = 1,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
    };

    attachments.push_back(inf);

    // just to test new framebuffer module.
    //offscreenFramebuffer = VulkanFramebuffer(vkDevice.getVmaAllocator(), {inf}, 3);
}

void VulkanApplication::createCommandPool()
{
    const auto poolInfo = [this]
    {
        VkCommandPoolCreateInfo pi{};
        pi.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pi.queueFamilyIndex = vkDevice.getGraphicsQueueIndice();
        pi.flags = 0;
        return pi;
    }();

    if (vkCreateCommandPool(vkDevice.getDevice(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create command pool.");
}

void VulkanApplication::createCommandBuffers()
{
    commandBuffers.resize(vkSwapchainFramebuffer.size());

    const auto allocateInfo = [this]
    {
        VkCommandBufferAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ai.commandPool = commandPool;
        // Primary buffer - can be executed directly
        // Secondary - cannot be executed directly, but can be called from Primary buffers.
        ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandBufferCount = vkSwapchainFramebuffer.size();

        return ai;
    }();

    if (vkAllocateCommandBuffers(vkDevice.getDevice(), &allocateInfo, commandBuffers.data()) != VK_SUCCESS)
        throw std::runtime_error("cannot allocate command buffers!");
}

// So this part will need to be a part of main render engine,
// as it has to deal with a loop across all Renderables which will contain all meshes
// and we need to invoke a draw call on every one of those, rebinding vertex buffer offsets
void VulkanApplication::recordCommandBuffers()
{
    for(size_t i = 0; i < commandBuffers.size(); ++i)
    {
        const auto beginInfo = []
        {
            VkCommandBufferBeginInfo cbi{};
            cbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cbi.flags = 0;
            cbi.pInheritanceInfo = nullptr;

            return cbi;
        }();

        if(vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
            throw std::runtime_error("cannot begin command buffer.");

        VkClearValue clearValue = {0.0f, 0.0f, 0.0f, 1.0f};
        const auto renderPassInfo = [i, &clearValue, this]
        {
            VkRenderPassBeginInfo rbi{};
            rbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            rbi.renderPass = vkSwapchainFramebuffer.getRenderPass();
            rbi.framebuffer = vkSwapchainFramebuffer[i];

            rbi.renderArea.offset = {0, 0};
            rbi.renderArea.extent = vkSwapchain.getSwapchainExtent();

            rbi.clearValueCount = 1;
            rbi.pClearValues = &clearValue;

            return rbi;
        }();

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // those 3 calls + also descriptor set bindings are going to be done in loop for all the
        // different renderables
        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline.getHandle());

	    vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, triangle.getVkInfo().getpVkBuffer(),
                triangle.getVkInfo().getpOffset());


        vkCmdDraw(commandBuffers[i], triangle.vertexCount(), 1, 0, 0); // 3 vertices, 1 instance,
                                                  // offset of vertex, offset of instance

        vkCmdEndRenderPass(commandBuffers[i]);

        if(vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("failed to record command buffer.");
    }
}

void VulkanApplication::createSyncObjects()
{
    imageAvailableSemaphores.resize(maxFramesInFlight);
    renderFinishedSemaphores.resize(maxFramesInFlight);

    inFlightFences.resize(maxFramesInFlight);
    imagesInFlight.resize(vkSwapchainFramebuffer.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for(size_t i = 0; i < imageAvailableSemaphores.size(); ++i)
    {
        if (vkCreateSemaphore(vkDevice.getDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i])
                != VK_SUCCESS or
        vkCreateSemaphore(vkDevice.getDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i])
                != VK_SUCCESS or
        vkCreateFence(vkDevice.getDevice(), &fenceInfo, nullptr, &inFlightFences[i] ) != VK_SUCCESS)
            throw std::runtime_error("failed to create semaphores!");
    }
}
//
// poligon
struct UniformTest
{
    alignas(16) glm::mat4 model{};
    alignas(16) glm::mat4 view{};
    alignas(16) glm::mat4 proj{};
    alignas(16) float time;
};

void VulkanApplication::initVulkan()
{
    #ifdef NDEBUG
    	static constexpr bool enableValidationLayers = false;
    #else
    	static constexpr bool enableValidationLayers = true;
    #endif

	vkInstance = VkInstance(enableValidationLayers);
    createSurface();
	vkDevice = VulkanDevice(vkInstance.getInstance(), surface);
    triangle = loadTriangleAsMesh(vkDevice.getVmaAllocator());
    vkSwapchain = VulkanSwapchain(vkDevice, surface, window);
    vkSwapchainFramebuffer = VulkanFramebuffer(vkDevice.getVmaAllocator(), vkSwapchain, false);

    FramebufferAttachmentInfo inf =
    {
        .ci = {
            .width = 800,
            .height = 600,
            .layerCount = 1,
            .mipLevels = 1,
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
        },

        .type = EFramebufferAttachmentType::ATTACHMENT_COLOR
    };

    // just to test new framebuffer module.
    auto framebuf = VulkanFramebuffer(vkDevice.getVmaAllocator(), {inf}, 3);

    createGraphicsPipeline();
    createCommandPool();
    createCommandBuffers();
    recordCommandBuffers();

    createSyncObjects();
}

void VulkanApplication::mainLoop()
{
	while (not glfwWindowShouldClose(window))
	{
		glfwPollEvents();
        drawFrame();
	}
}

void VulkanApplication::drawFrame()
{
    // we check whether we can actually start rendering another frame, we want to have
    // a maximum of maxFramesInFlight on queue.
    vkWaitForFences(vkDevice.getDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(vkDevice.getDevice(),
                          vkSwapchain.getSwapchain(),
                          UINT64_MAX, // timeout in ns, uint64_max disables timeout.
                          imageAvailableSemaphores[currentFrame],
                          VK_NULL_HANDLE, //fence, if applicable.
                          &imageIndex);

    // The first fence is not enough of a synchronization point, as the vkAcquireNextImageKHR
    // can actually give us frames out-of-order, or we can have less images in swapchain
    // than our maximum frames in flight setting. Therefore we also have to check if given
    // swapchain image is not used by another frame, and if so, we have to stall.
    if(imagesInFlight[imageIndex] != VK_NULL_HANDLE)
        vkWaitForFences(vkDevice.getDevice(), 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);

    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    // The inFlightFences[currentFrame] is lit now.
    // So is imagesInFlight[imageIndex], as they are one and the same fence.

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame] };

    const auto submitInfo = [&waitSemaphores, &waitStages, &signalSemaphores, imageIndex, this]
    {
        VkSubmitInfo submitInfo{};
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
    vkResetFences(vkDevice.getDevice(), 1, &inFlightFences[currentFrame]);

    // dont we have a race condition right there? Yes but this is not multithreaded. (yet!)
    if(vkQueueSubmit(vkDevice.getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame])
            != VK_SUCCESS)
        throw std::runtime_error("Cannot submit to queue");

    VkSwapchainKHR swapchains[] = {vkSwapchain.getSwapchain()};

    const auto presentInfo = [&signalSemaphores, &swapchains, &imageIndex, this]
    {
        VkPresentInfoKHR pi{};
        pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        pi.waitSemaphoreCount = 1;
        pi.pWaitSemaphores = signalSemaphores;

        pi.swapchainCount = 1;
        pi.pSwapchains = swapchains;
        pi.pImageIndices = &imageIndex;

        return pi;
    }();

    vkQueuePresentKHR(vkDevice.getPresentationQueue(), &presentInfo);

    currentFrame = (currentFrame + 1) % maxFramesInFlight;
}

void VulkanApplication::cleanup()
{
    for(size_t i = 0; i < imageAvailableSemaphores.size(); ++i)
    {
        vkDestroySemaphore(vkDevice.getDevice(), imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(vkDevice.getDevice(), renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(vkDevice.getDevice(), inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(vkDevice.getDevice(), commandPool, nullptr);

    //for(auto&& framebuffer : swapChainFramebuffers)
    //    vkDestroyFramebuffer(vkDevice.getDevice(), framebuffer, nullptr);

    //vkDestroyRenderPass(vkDevice.getDevice(), renderPass, nullptr);

    for (auto&& image : vkSwapchain.getSwapchainImageViews())
        vkDestroyImageView(vkDevice.getDevice(), image, nullptr);

    vkDestroySwapchainKHR(vkDevice.getDevice(), vkSwapchain.getSwapchain(), nullptr);
    vkDestroySurfaceKHR(vkInstance.getInstance(), surface, nullptr);
    vkDestroyDevice(vkDevice.getDevice(), nullptr);
	glfwDestroyWindow(window);
	glfwTerminate();
}

} // namespace render

