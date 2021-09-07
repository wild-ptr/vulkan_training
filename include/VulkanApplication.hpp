#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <optional>
#include <vector>

#include "Mesh.hpp"
#include "Pipeline.hpp"
#include "VulkanDevice.hpp"
#include "VulkanFramebuffer.hpp"
#include "VulkanInstance.hpp"
#include "VulkanSwapchain.hpp"
#include "Renderable.hpp"
#include "TextureManager.hpp"
#include "AssetLoader.hpp"
#include "PerFrameUniformSystem.hpp"
#include "CameraSystem.hpp"

namespace render {

class VulkanApplication {
public:
    void run();

private:
    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup(); // no RAII for now.
    void createSurface();
    void createGraphicsPipeline();
    void createOffscreenFramebuffer();
    void createCommandPool();
    void createCommandBuffers();
    void recordCommandBuffers(uint32_t swapchainImageIdx, uint32_t frameInFlightIdx);
    void createSyncObjects();

    void drawFrame();

    void updateUbos(size_t frameIdx);
    void render();
    void sendBufferToQueue(uint32_t imageIndex, size_t inFlightFrameNo);

    // downright retarded.
    const size_t WIDTH = 1500;
    const size_t HEIGHT = 1000;

    GLFWwindow* window;
    VkSurfaceKHR surface;

    VmaAllocator vmaAllocator;
    VulkanInstance vkInstance;

    // @TODO: make all of them shared_ptrs in the future and delete default ctors.
    std::shared_ptr<VulkanDevice> vkDevice;
    VulkanSwapchain vkSwapchain;
    VulkanFramebuffer vkSwapchainFramebuffer;
    std::shared_ptr<memory::TextureManager> textureManager;
    std::shared_ptr<CameraSystem> cameraSystem;
    std::shared_ptr<AssetLoader> assetLoader;
    std::shared_ptr<Pipeline> pipeline;
    std::shared_ptr<memory::PerFrameUniformSystem> perFrameData;

    std::shared_ptr<Renderable> to_render_test;

    // @TODO: change amount of commandBuffers to consts::maxFramesInFlight
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    // move to its own class later?
    struct FrameSyncData
    {
        size_t currentFrame { 0 };
        std::array<VkSemaphore, consts::maxFramesInFlight> imageAvailableSem;
        std::array<VkSemaphore, consts::maxFramesInFlight> renderFinishedSem;
        std::array<VkFence, consts::maxFramesInFlight> inFlightFences;
        std::vector<VkFence> imagesInFlight;
        std::shared_ptr<VulkanDevice> device;

        FrameSyncData(std::shared_ptr<VulkanDevice> device_ptr, size_t swapchainImageCnt)
            : imagesInFlight(swapchainImageCnt)
            , device(std::move(device_ptr))
        {
            VkSemaphoreCreateInfo semaphoreInfo =
            {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            };

            VkFenceCreateInfo fenceInfo =
            {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .flags = VK_FENCE_CREATE_SIGNALED_BIT,
            };

            for(size_t i = 0; i < consts::maxFramesInFlight; ++i)
            {
                vkCreateSemaphore(device->getDevice(), &semaphoreInfo, nullptr, &imageAvailableSem[i]);
                vkCreateSemaphore(device->getDevice(), &semaphoreInfo, nullptr, &renderFinishedSem[i]);
                vkCreateFence(device->getDevice(), &fenceInfo, nullptr, &inFlightFences[i]);
            }
        }

        void advanceFrame()
        {
            ++currentFrame;
            if (currentFrame % consts::maxFramesInFlight == 0)
            {
                currentFrame = 0;
            }
        }

        size_t getFrameIndex() { return currentFrame; }

    };

    std::shared_ptr<FrameSyncData> frameSyncData;
};

} // namespace vk_main
