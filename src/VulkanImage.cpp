#include <algorithm>
#include <array>
#include <cassert>

#include "VulkanImage.hpp"
#include "VulkanMacros.hpp"
#include "VmaVulkanBuffer.hpp"

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

VulkanImage::VulkanImage(const VulkanImageCreateInfo& ci, std::shared_ptr<VulkanDevice> deviceptr)
    : device(std::move(deviceptr))
    , creationData(ci)
{
    format = ci.format;

    VkImageAspectFlags aspectMask = 0;

    if (ci.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
        aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    else if (ci.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        if (hasDepth()) {
            aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT; //VK_IMAGE_ASPECT_COLOR_BIT;
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
        imgCi.tiling = ci.tiling;
        imgCi.usage = ci.usage;
        imgCi.initialLayout = ci.initialLayout;

        return imgCi;
    }();

    VmaAllocationCreateInfo vmaAllocInfo { .usage = VMA_MEMORY_USAGE_GPU_ONLY };
    VK_CHECK(vmaCreateImage(device->getVmaAllocator(), &imageCi, &vmaAllocInfo, &vkImage, &allocation, &allocationInfo));

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

    VK_CHECK(vkCreateImageView(device->getDevice(), &imageViewCi, nullptr, &vkImageView));
}

VulkanImage::VulkanImage(const VulkanImageCreateInfo& ci, std::shared_ptr<VulkanDevice> deviceptr,
            const void* data, size_t size)
    : VulkanImage(ci, std::move(deviceptr))
{
    transitionLayoutBarrier(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyToImageStaging(data, size);
    transitionLayoutBarrier(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

// this will need changes to subresourceRange element. Or will it?
void VulkanImage::transitionLayoutBarrier(VkImageLayout from, VkImageLayout to)
{
    assert(device and not swapchainImage);
    assert(creationData.usage & (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));

    device->immediateSubmitBlocking(
            [from, to, this](VkCommandBuffer cmd)
            {
		        VkImageMemoryBarrier imageBarrier_toTransfer = {};
		        imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

		        imageBarrier_toTransfer.oldLayout = from;
		        imageBarrier_toTransfer.newLayout = to;
		        imageBarrier_toTransfer.image = getImage();
		        imageBarrier_toTransfer.subresourceRange = getSubresourceRange();

		        imageBarrier_toTransfer.srcAccessMask = 0;
		        imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		        vkCmdPipelineBarrier(cmd,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);
            });
}

void VulkanImage::copyToImageStaging(const void* data, size_t size)
{
    assert(device and not swapchainImage);
    assert(creationData.usage & (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));

    VmaVulkanBuffer stagingBuffer(device, data, size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    // This does not understand mipmaps yet. Neither do i know how to use them in vulkan yet.
    device->immediateSubmitBlocking(
            [stagingBuffer, this](VkCommandBuffer cmd)
            {
                VkBufferImageCopy copyRegion = {};
	            copyRegion.bufferOffset = 0;
	            copyRegion.bufferRowLength = 0;
	            copyRegion.bufferImageHeight = 0;

	            copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	            copyRegion.imageSubresource.mipLevel = 0;
	            copyRegion.imageSubresource.baseArrayLayer = 0;
	            copyRegion.imageSubresource.layerCount = 1;
	            copyRegion.imageExtent = VkExtent3D{
                    .width = creationData.width,
                    .height = creationData.height,
                    .depth = 1
                    };

	                //copy the buffer into the image
	            vkCmdCopyBufferToImage(cmd, stagingBuffer.getVkBuffer(), getImage(),
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
            });
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
    , swapchainImage(true)
{
}

} // namespace render::memory
