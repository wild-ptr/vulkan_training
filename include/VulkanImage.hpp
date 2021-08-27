#pragma once
#define GLFW_INCLUDE_VULKAN
#include "VulkanDevice.hpp"
#include "utils/UniqueHandle.hpp"
#include "vk_mem_alloc.h"
#include <GLFW/glfw3.h>
#include <memory>

namespace render::memory {

struct VulkanImageCreateInfo {
    uint32_t width, height;
    uint32_t layerCount;
    uint32_t mipLevels;
    VkFormat format;
    VkImageUsageFlags usage;
    VkSampleCountFlagBits imageSampleCount = VK_SAMPLE_COUNT_1_BIT;
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
};

class VulkanImage {
public:
    VulkanImage(const VulkanImageCreateInfo& ci, std::shared_ptr<VulkanDevice> device);
    VulkanImage(const VulkanImageCreateInfo& ci, std::shared_ptr<VulkanDevice> device,
            const void* data, size_t size);

    // ctor for wrapping previously allocated images (swapchain)
    VulkanImage(VkImage image, VkImageView imageView, VkFormat format, VkImageSubresourceRange range);
    //~VulkanImage(); // @TODO
    bool hasDepth();
    bool hasStencil();
    bool hasDepthOrStencil();

    VkImage getImage() { return vkImage; }
    VkImageView getImageView() { return vkImageView; }
    VkFormat getImageFormat() { return format; }
    VkImageSubresourceRange getSubresourceRange() { return subresourceRange; }
    const VulkanImageCreateInfo& getCreationData() { return creationData; }

private:
    void transitionLayoutBarrier(VkImageLayout from, VkImageLayout to);
    void copyToImageStaging(const void* data, size_t size);

    std::shared_ptr<VulkanDevice> device;
    VmaAllocation allocation { VK_NULL_HANDLE };
    VmaAllocationInfo allocationInfo {};

    VkImage vkImage;
    VkImageView vkImageView;
    VkFormat format;
    VkImageSubresourceRange subresourceRange;
    VulkanImageCreateInfo creationData{};
    bool swapchainImage{false};
};

} // namespace render::memory
