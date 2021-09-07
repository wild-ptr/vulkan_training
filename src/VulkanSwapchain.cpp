#include "VulkanSwapchain.hpp"
#include "Logger.hpp"
#include "VulkanDevice.hpp"

namespace {

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
    for (auto&& format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return format;
    }

    return formats[0]; // fallback, should not happen.
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes)
{
    for (auto&& presentMode : presentModes) {
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return presentMode;
    }

    return VK_PRESENT_MODE_FIFO_KHR; // guaranteed availability.
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window)
{

    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        // clamp between minimum and maximum extents supported by implementation
        // seems like unnecessary step in 99.9% of cases.
        actualExtent.width = std::max(capabilities.minImageExtent.width,
            std::min(capabilities.maxImageExtent.width,
                actualExtent.width));

        actualExtent.height = std::max(capabilities.minImageExtent.width,
            std::min(capabilities.maxImageExtent.height,
                actualExtent.height));

        return actualExtent;
    }
}

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount,
            details.presentModes.data());
    }

    return details;
}

void createSwapChainAndImageFormatExtent(
    const render::VulkanDevice& vkDevice,
    VkSurfaceKHR surface,
    GLFWwindow* window,
    VkSwapchainKHR& swapChain_out,
    VkFormat& swapChainImageFormat_out,
    VkExtent2D& extent_out)
{
    auto swapChainSupport = querySwapChainSupport(vkDevice.getPhysicalDevice(), surface);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);

    extent_out = chooseSwapExtent(swapChainSupport.capabilities, window);
    swapChainImageFormat_out = surfaceFormat.format;

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount;

    if (swapChainSupport.capabilities.maxImageCount > 0 and imageCount > swapChainSupport.capabilities.maxImageCount)
        imageCount = swapChainSupport.capabilities.maxImageCount;

    const auto createInfo = [&]() {
        VkSwapchainCreateInfoKHR createInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent_out;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        auto indices = vkDevice.getQueueIndices();

        uint32_t queueFamilyIndices[] = {
            indices.graphicsFamily.value(),
            indices.presentationFamily.value()
        };

        if (indices.graphicsFamily != indices.presentationFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        // we dont want any transformations.
        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

        // blending with other windows in window system, we want to ignore alpha channel.
        // Otherwise our application would become semi-transparent except of what we are rendering
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE; // we dont care about pixels that are obscured by oth windows
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        return createInfo;
    }();

    dbgI << "attempting to create swapchain." << NEWL;

    if (vkCreateSwapchainKHR(vkDevice.getDevice(), &createInfo, nullptr, &swapChain_out) != VK_SUCCESS)
        throw std::runtime_error("Failed to create swapchain.");
}

std::vector<VkImage> createSwapchainImages(const render::VulkanDevice& vkDevice, VkSwapchainKHR swapChain)
{
    // And there we get vkImage handles from swapchain.
    std::vector<VkImage> swapChainImages;

    uint32_t imagesCount;
    vkGetSwapchainImagesKHR(vkDevice.getDevice(), swapChain, &imagesCount, nullptr);

    swapChainImages.resize(imagesCount);
    vkGetSwapchainImagesKHR(vkDevice.getDevice(), swapChain, &imagesCount, swapChainImages.data());

    return swapChainImages;
}

std::vector<VkImageView> createSwapchainImageViews(
    const render::VulkanDevice& vkDevice,
    const VkFormat& swapChainImageFormat,
    const std::vector<VkImage>& swapChainImages)
{
    std::vector<VkImageView> swapChainImageViews;
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); ++i) {
        const auto createInfo = [&] {
            VkImageViewCreateInfo ci {};
            ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            ci.image = swapChainImages[i];
            ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
            ci.format = swapChainImageFormat;

            ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            ci.subresourceRange.baseMipLevel = 0;
            ci.subresourceRange.levelCount = 1;
            ci.subresourceRange.baseArrayLayer = 0;
            ci.subresourceRange.layerCount = 1;

            return ci;
        }();

        if (vkCreateImageView(vkDevice.getDevice(), &createInfo, nullptr, &swapChainImageViews[i])
            != VK_SUCCESS)
            throw std::runtime_error("Failed to create swapchain.");
    }

    return swapChainImageViews;
}

VkImageSubresourceRange createSwapchainSubresourceRange()
{
    return []() {
        VkImageSubresourceRange srr {};
        srr.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        srr.levelCount = 1;
        srr.layerCount = 1;

        return srr;
    }();
}

} // anon namespace

namespace render {
VulkanSwapchain::VulkanSwapchain(
    const VulkanDevice& device,
    VkSurfaceKHR surface,
    GLFWwindow* window)
{
    createSwapChainAndImageFormatExtent(device, surface, window,
        this->swapChain, this->swapChainImageFormat, this->swapChainExtent);

    swapChainImages = createSwapchainImages(device, swapChain);
    swapChainImageViews = createSwapchainImageViews(device, swapChainImageFormat, swapChainImages);
    swapChainSubresourceRange = createSwapchainSubresourceRange();
}

} // namespace render
