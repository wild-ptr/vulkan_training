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
	//~VulkanImage(); // @TODO
	bool hasDepth();
	bool hasStencil();
	bool hasDepthOrStencil();

    VkImage getImage() { return vkImage; }
    VkImageView getImageView() { return vkImageView; }
    VkFormat getImageFormat() { return format; }
    VkImageSubresourceRange getSubresourceRange() { return subresourceRange; }

private:
	VmaAllocator allocator;
	VmaAllocation allocation;
	VmaAllocationInfo allocationInfo;

	VkImage vkImage;
	VkImageView vkImageView;
	VkFormat format;
	VkImageSubresourceRange subresourceRange;
};

} // namespace render::memory
