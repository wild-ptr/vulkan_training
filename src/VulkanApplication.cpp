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

namespace
{

static constexpr auto maxFramesInFlight = 2u;

VmaAllocator createVmaAllocator(
        VkInstance instance,
        VkPhysicalDevice physicalDevice,
        VkDevice device)
{
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_0;
    allocatorInfo.physicalDevice = physicalDevice;
    allocatorInfo.device = device;
    allocatorInfo.instance = instance;

    VmaAllocator allocator;
    vmaCreateAllocator(&allocatorInfo, &allocator);

    return allocator;
}


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

    pipeline = Pipeline(shaders, vkSwapchain.getSwapchainExtent(), vkDevice.getDevice(), renderPass,
            Pipeline::vertex_input_tag<Vertex>{});
}

void VulkanApplication::createRenderPass()
{
    // so this is kinda our render target, we render to color buffer.
    const auto colorAttachment = [this]
    {
        VkAttachmentDescription ca{}; // DESCRIPTION
        ca.format = vkSwapchain.getSwapchainImageFormat();
        ca.samples = VK_SAMPLE_COUNT_1_BIT; // this is used for MSAA.

        ca.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // reset background to black per frame.
        ca.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // store results to be read later to present.

        ca.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        ca.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        ca.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // dont care about previous image layout.
        ca.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // ready for presentation after render.

        return ca;
    }();

    // we will create a simple, one subpass run.

    // attachments references are used by subpasses,
    // so we specify that for subpasses using this reference, we want image
    // to transition to color optimal layout before rendering using the given subpass.
    const auto colorAttachmentRef = []
    {
        VkAttachmentReference car{}; // REFERENCE TO ATTACHMENT
        car.attachment = 0;
        car.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        return car;
    }(); // so this subpass is rendering to color attachment. Easy enough.

    // Now this is important, we bind fragment shader output to render target there.
    const auto subpass = [&colorAttachmentRef]
    {
        VkSubpassDescription sp{};
        sp.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        sp.colorAttachmentCount = 1;
        sp.pColorAttachments = &colorAttachmentRef; // location 0 in our fragshader, since its [1] array

        return sp;
    }();

    const auto subpassDependency = [this]
    {
        VkSubpassDependency sd{};
        sd.srcSubpass = VK_SUBPASS_EXTERNAL; // implicit subpass before dstSubpass.
        sd.dstSubpass = 0; // index of subpass

        // The renderpass has two implicit stages, one at the end of the render pass
        // to transition image into presentation layout, and one at the beginning to
        // transition newly acquired image to beginning layout.
        // The renderpass will do this in the very moment we start our pipeline,
        // and this is too early, as we still havent acquired an image at this point.
        // So we tell renderpass to wait with memory transitions untill we get to fragment shader,
        // as then (as we specified with semaphores during submission) image must be available to us.
        // src - wait on this stage to happen
        sd.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        sd.srcAccessMask = 0;

        // Color attachment write operation in color attachment stage
        // should wait for transition before writing.
        // what should wait on src - in our case attachment writing should wait.
        sd.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        sd.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        return sd;
    }();

    const auto renderPassInfo = [&colorAttachment, &subpassDependency, &subpass]
    {
        VkRenderPassCreateInfo rpi{};
        rpi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rpi.attachmentCount = 1;
        rpi.pAttachments = &colorAttachment;
        rpi.subpassCount = 1;
        rpi.pSubpasses = &subpass;

        rpi.dependencyCount = 1;
        rpi.pDependencies = &subpassDependency;

        return rpi;
    }();

    if (vkCreateRenderPass(vkDevice.getDevice(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
        throw std::runtime_error("Cannot create render pass!");
}

// We can actually plug in multiple vkImages into the renderpass right there.
// Framebuffers depend on renderpass for ceation.
void VulkanApplication::createFramebuffers()
{
    swapChainFramebuffers.resize(vkSwapchain.getSwapchainImageViews().size());

    for(size_t i = 0; i < vkSwapchain.getSwapchainImageViews().size(); ++i)
    {
        VkImageView attachments[] = {
            vkSwapchain.getSwapchainImageViews()[i]
        };

        const auto framebufferInfo = [&attachments, this]
        {
            VkFramebufferCreateInfo fi{};
            fi.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fi.renderPass = renderPass;
            fi.attachmentCount = 1;
            fi.pAttachments = attachments;
            fi.width = vkSwapchain.getSwapchainExtent().width;
            fi.height = vkSwapchain.getSwapchainExtent().height;
            fi.layers = 1;

            return fi;
        }();

        if (vkCreateFramebuffer(vkDevice.getDevice(), &framebufferInfo, nullptr, &swapChainFramebuffers[i])
            != VK_SUCCESS)
            throw std::runtime_error("Cannot create framebuffers.");
    }
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
    commandBuffers.resize(swapChainFramebuffers.size());

    const auto allocateInfo = [this]
    {
        VkCommandBufferAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ai.commandPool = commandPool;
        // Primary buffer - can be executed directly
        // Secondary - cannot be executed directly, but can be called from Primary buffers.
        ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandBufferCount = swapChainFramebuffers.size();

        return ai;
    }();

    if (vkAllocateCommandBuffers(vkDevice.getDevice(), &allocateInfo, commandBuffers.data()) != VK_SUCCESS)
        throw std::runtime_error("cannot allocate command buffers!");
}

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
            rbi.renderPass = renderPass;
            rbi.framebuffer = swapChainFramebuffers[i];

            rbi.renderArea.offset = {0, 0};
            rbi.renderArea.extent = vkSwapchain.getSwapchainExtent();

            rbi.clearValueCount = 1;
            rbi.pClearValues = &clearValue;

            return rbi;
        }();

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline.getHandle());

        VkDeviceSize offset = 0;
	    vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &triangle.getBufferInfo().memory_buffer, &offset);


        vkCmdDraw(commandBuffers[i], 3, 1, 0, 0); // 3 vertices, 1 instance,
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
    imagesInFlight.resize(swapChainFramebuffers.size(), VK_NULL_HANDLE);

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


    vmaAllocator = createVmaAllocator(
        vkInstance.getInstance(),
        vkDevice.getPhysicalDevice(),
        vkDevice.getDevice());

    triangle = loadTriangleAsMesh(vmaAllocator);

    vkSwapchain = VulkanSwapchain(vkDevice, surface, window);

    createRenderPass();
    createGraphicsPipeline();

    createFramebuffers();

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

    // then we reset the fence, so we can stall
    vkResetFences(vkDevice.getDevice(), 1, &inFlightFences[currentFrame]);

    // dont we have a race condition there? Yes but this is not multithreaded.
    // easy.
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

    for(auto&& framebuffer : swapChainFramebuffers)
        vkDestroyFramebuffer(vkDevice.getDevice(), framebuffer, nullptr);

    vkDestroyRenderPass(vkDevice.getDevice(), renderPass, nullptr);

    for (auto&& image : vkSwapchain.getSwapchainImageViews())
        vkDestroyImageView(vkDevice.getDevice(), image, nullptr);

    vkDestroySwapchainKHR(vkDevice.getDevice(), vkSwapchain.getSwapchain(), nullptr);
    vkDestroySurfaceKHR(vkInstance.getInstance(), surface, nullptr);
    vkDestroyDevice(vkDevice.getDevice(), nullptr);
	glfwDestroyWindow(window);
	glfwTerminate();
}



} // namespace render

