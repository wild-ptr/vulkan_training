#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VulkanImage.hpp"
#include "vk_mem_alloc.h"

namespace render
{
class VulkanFramebuffer
{
public:
	// usually we will want as many as we have swapchain images.
	VulkanFramebuffer(
			VmaAllocator allocator,
			std::vector<memory::VulkanImageCreateInfo> ci,
			size_t numOfFramebuffers);

	VkFramebuffer& operator[](size_t i)
	{
		return framebuffers[i].vkFramebuffer;
	}

private:


	struct Attachment
	{
		memory::VulkanImage image;
		VkAttachmentDescription description;
	};

	struct Framebuffer
	{
		VkFramebuffer vkFramebuffer;
		std::vector<Attachment> attachments;
	};

	std::vector<Framebuffer> framebuffers;

};

} // namespace render
