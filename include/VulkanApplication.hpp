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
#include "PerFrameUniformSystem.hpp"

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
    void recordCommandBuffers();
    void createSyncObjects();

    void drawFrame();

    // members
    const size_t WIDTH = 1024;
    const size_t HEIGHT = 768;

    GLFWwindow* window;
    VkSurfaceKHR surface;

    VmaAllocator vmaAllocator;
    VulkanInstance vkInstance;

    // @TODO: make all of them shared_ptrs in the future and delete default ctors.
    std::shared_ptr<VulkanDevice> vkDevice;
    VulkanSwapchain vkSwapchain;
    VulkanFramebuffer vkSwapchainFramebuffer;
    std::shared_ptr<memory::TextureManager> textureManager;

    Mesh triangle;
    Mesh triangle2;
    std::unique_ptr<Renderable> two_triangles;

    std::shared_ptr<Pipeline> pipeline;

    std::shared_ptr<memory::PerFrameUniformSystem> perFrameData;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;

    size_t currentFrame { 0 };
};

} // namespace vk_main
