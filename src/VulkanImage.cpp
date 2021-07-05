#include <algorithm>
#include <array>
#include <cassert>

#include "VulkanImage.hpp"
#include "VulkanMacros.hpp"

namespace render::memory {

bool VulkanImage::hasDepth()
{
    static std::array<VkFormat, 6> formats = {
        VK_FORMAT_D16_UNORM,
        VK_FORMAT_X8_D24_UNORM_PACK32,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
    };

    return std::find(std::begin(formats), std::end(formats), format) != std::end(formats);
}

bool VulkanImage::hasStencil()
{
    static std::array<VkFormat, 4> formats = {
        VK_FORMAT_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
    };

    return std::find(formats.begin(), formats.end(), format) != std::end(formats);
}

bool VulkanImage::hasDepthOrStencil()
{
    return hasDepth() or hasStencil();
}

VulkanImage::VulkanImage(const VulkanImageCreateInfo& ci, VmaAllocator allocator)
{
    format = ci.format;

    VkImageAspectFlags aspectMask = 0;

    if (ci.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
        aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    else if (ci.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        if (hasDepth()) {
            aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }
        if (hasStencil()) {
            aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }

    assert(aspectMask > 0);

    const auto imageCi = [&ci]() {
        VkImageCreateInfo imgCi {};
        imgCi.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;

        imgCi.imageType = VK_IMAGE_TYPE_2D;
        imgCi.format = ci.format;
        imgCi.extent.width = ci.width;
        imgCi.extent.height = ci.height;
        imgCi.extent.depth = 1;
        imgCi.mipLevels = ci.mipLevels;
        imgCi.arrayLayers = ci.layerCount;
        imgCi.samples = ci.imageSampleCount;
        imgCi.tiling = VK_IMAGE_TILING_OPTIMAL;
        imgCi.usage = ci.usage;

        return imgCi;
    }();

    VmaAllocationCreateInfo vmaAllocInfo { .usage = VMA_MEMORY_USAGE_GPU_ONLY };
    VK_CHECK(vmaCreateImage(allocator, &imageCi, &vmaAllocInfo, &vkImage, &allocation, &allocationInfo));

    subresourceRange = [aspectMask, &ci]() {
        VkImageSubresourceRange srr {};
        srr.aspectMask = aspectMask;
        srr.levelCount = 1;
        srr.layerCount = ci.layerCount;

        return srr;
    }();

    // Implicitly inherits usage from VkImage
    const auto imageViewCi = [&ci, aspectMask, this]() {
        VkImageViewCreateInfo ivci {};
        ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ivci.viewType = (ci.layerCount == 1) ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        ivci.format = ci.format;
        ivci.subresourceRange = subresourceRange;
        ivci.subresourceRange.aspectMask = hasDepth() ? VK_IMAGE_ASPECT_DEPTH_BIT : aspectMask;
        ivci.image = vkImage;

        return ivci;
    }();

    VmaAllocatorInfo allocInfo;
    vmaGetAllocatorInfo(allocator, &allocInfo);

    VK_CHECK(vkCreateImageView(allocInfo.device, &imageViewCi, nullptr, &vkImageView));
}

// just a wrapper for externally created vkImages, like we get from the swapchain
VulkanImage::VulkanImage(
    VkImage image,
    VkImageView imageView,
    VkFormat format,
    VkImageSubresourceRange range)
    : vkImage(image)
    , vkImageView(imageView)
    , format(format)
    , subresourceRange(range)
    , wrappedAllocatedImage(true)
{
}

} // namespace render::memory
