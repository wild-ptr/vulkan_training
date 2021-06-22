#include "VulkanFramebuffer.hpp"

namespace render
{
VulkanFramebuffer::VulkanFramebuffer(VmaAllocator allocator, std::vector<memory::VulkanImageCreateInfo> ci, size_t numOfFramebuffers)
{
	for(size_t framebufferIdx = 0; framebufferIdx < numOfFramebuffers; ++ framebufferIdx)
	{
		Framebuffer framebuffer{};

		for(auto& attachmentInfo : ci)
		{

			Attachment attachment =
			{
				.image = memory::VulkanImage(attachmentInfo, allocator),

				.description = {
					.format = attachmentInfo.format,
					.samples = attachmentInfo.imageSampleCount,
					.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
					.storeOp = (attachmentInfo.usage & VK_IMAGE_USAGE_SAMPLED_BIT) ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE,
					.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE, // shouldnt we clear it?
					.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
					.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.finalLayout = attachment.image.hasDepthOrStencil() ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL :
																		  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
					}
			};

			framebuffer.attachments.emplace_back(std::move(attachment));
		}

		framebuffers.emplace_back(framebuffer);
	}

	// after the Framebuffer vector is ready and we have all attachments in check, we can create a renderpass based on that,
	// and then finally the VkFramebuffer objects.
}


} // namespace render
