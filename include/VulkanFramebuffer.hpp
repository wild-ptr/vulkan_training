#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VulkanImage.hpp"
#include "vk_mem_alloc.h"

namespace render
{

enum class EFramebufferAttachmentType
{
    ATTACHMENT_COLOR,
    ATTACHMENT_DEPTH,
    ATTACHMENT_OFFSCREEN_ALBEDO,
    ATTACHMENT_OFFSCREEN_NORMALS,
    ATTACHMENT_OFFSCREEN_POSITION,
    ATTACHMENT_OFFSCREEN_TANGENT
};

struct FramebufferAttachmentInfo
{
    memory::VulkanImageCreateInfo ci;
    EFramebufferAttachmentType type;
};

class VulkanFramebuffer
{
public:
	// usually we will want as many as we have swapchain images.
	VulkanFramebuffer(
			VmaAllocator allocator,
			std::vector<FramebufferAttachmentInfo> ci,
			size_t numOfFramebuffers);

	VkFramebuffer& operator[](size_t i)
	{
		return framebuffers[i].vkFramebuffer;
	}

    VkAttachmentDescription getAttachmentDescription(EFramebufferAttachmentType type);
    VkRenderPass getRenderPass() { return renderPass; }


private:
    void createRenderPass();
    void createFramebuffer();

	struct Framebuffer
	{
		VkFramebuffer vkFramebuffer = VK_NULL_HANDLE;
		std::vector<memory::VulkanImage> attachments;
	};

    // same storage order as attachments vector.
    struct AttachmentInfo
    {
		VkAttachmentDescription description;
        EFramebufferAttachmentType type;
    };

    VmaAllocator allocator;
	std::vector<Framebuffer> framebuffers;
    std::vector<AttachmentInfo> attachmentsInfo;
    VkRenderPass renderPass;
    uint32_t width, height;
};

} // namespace render
