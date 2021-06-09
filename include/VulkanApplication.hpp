#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <optional>
#include <vector>

#include "Pipeline.hpp"
#include "VulkanDevice.hpp"
#include "VulkanInstance.hpp"
#include "VulkanSwapchain.hpp"

namespace render
{


class VulkanApplication
{
public:
	void run();

private:
	void initWindow();
	void initVulkan();
	void mainLoop();
	void cleanup(); // no RAII for now.
    void createSurface();
    void createSwapChain();
    void createImageViews();
    void createGraphicsPipeline();
    void createRenderPass();
    VkShaderModule createShaderModule(const std::vector<char>& code);
    void createFramebuffers();
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

	VulkanInstance vkInstance;
	VulkanDevice vkDevice;

    VulkanSwapchain vkSwapchain;

    //VkSwapchainKHR swapChain;
    //VkFormat swapChainImageFormat;
    //VkExtent2D swapChainExtent;
    //
    //std::vector<VkImage> swapChainImages;
    //std::vector<VkImageView> swapChainImageViews;

    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    Pipeline pipeline;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;

    size_t currentFrame{0};
};

} // namespace vk_main
