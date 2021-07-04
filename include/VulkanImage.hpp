#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VulkanDevice.hpp"
#include "vk_mem_alloc.h"

namespace render::memory
{

struct VulkanImageCreateInfo
{
	uint32_t width, height;
	uint32_t layerCount;
	uint32_t mipLevels;
	VkFormat format;
	VkImageUsageFlags usage;
	VkSampleCountFlagBits imageSampleCount = VK_SAMPLE_COUNT_1_BIT;
};


class VulkanImage
{
public:
	VulkanImage(const VulkanImageCreateInfo& ci, VmaAllocator allocator);

    // ctor for wrapping previously allocated images.
    VulkanImage(VkImage image, VkImageView imageView, VkFormat format, VkImageSubresourceRange range);
	//~VulkanImage(); // @TODO
	bool hasDepth();
	bool hasStencil();
	bool hasDepthOrStencil();

    VkImage getImage() { return vkImage; }
    VkImageView getImageView() { return vkImageView; }
    VkFormat getImageFormat() { return format; }
    VkImageSubresourceRange getSubresourceRange() { return subresourceRange; }

private:
    // will be used to change dtor behaviour to noop
    bool wrappedAllocatedImage {false};

	VmaAllocator allocator {VK_NULL_HANDLE};
	VmaAllocation allocation {VK_NULL_HANDLE};
	VmaAllocationInfo allocationInfo {};

	VkImage vkImage;
	VkImageView vkImageView;
	VkFormat format;
	VkImageSubresourceRange subresourceRange;
};

} // namespace render::memory
