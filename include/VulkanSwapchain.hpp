#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

namespace render
{
class VulkanDevice;

class VulkanSwapchain
{
public:
    VulkanSwapchain() = default;
    VulkanSwapchain(const VulkanDevice& device, VkSurfaceKHR surface, GLFWwindow* window);
    ~VulkanSwapchain() = default; // todo later

    const VkFormat& getSwapchainImageFormat() const { return swapChainImageFormat; }
    const VkExtent2D& getSwapchainExtent() const { return swapChainExtent; }
    VkSwapchainKHR getSwapchain() const { return swapChain; }
    const std::vector<VkImage>& getSwapchainImages() const { return swapChainImages; }
    const std::vector<VkImageView>& getSwapchainImageViews() const { return swapChainImageViews; }
    size_t size() const { return swapChainImages.size(); }

    const VkImageSubresourceRange& getSwapchainSubresourceRange() const
    {
        return swapChainSubresourceRange;
    }

private:
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    VkImageSubresourceRange swapChainSubresourceRange;
};

} // namespace render
