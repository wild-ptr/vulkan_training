#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VulkanImage.hpp"
#include "VulkanSwapchain.hpp"
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
    ATTACHMENT_OFFSCREEN_TANGENT,
    ATTACHMENT_SWAPCHAIN_PRESENT_COLOR,
};

struct FramebufferAttachmentInfo
{
    memory::VulkanImageCreateInfo ci;
    EFramebufferAttachmentType type;
};

class VulkanFramebuffer
{
public:
	VulkanFramebuffer(
			VmaAllocator allocator,
			std::vector<FramebufferAttachmentInfo> ci,
			size_t numOfFramebuffers);

    // this ctor is used to create present framebuffers, one per swapchain.
    // Implicitly creates a depth attachment. Per swapchain image now but this can change in the
    // future, as all we need is a single depth attachment.
    VulkanFramebuffer(
            VmaAllocator allocator,
            const VulkanSwapchain& swapchain,
            bool createDepthAttachment);

    VulkanFramebuffer() = default;

    VkAttachmentDescription getAttachmentDescription(EFramebufferAttachmentType type);
    VkRenderPass getRenderPass() { return renderPass; }

    size_t size() { return framebuffers.size(); }

	VkFramebuffer& operator[](size_t i)
	{
		return framebuffers[i].vkFramebuffer;
	}

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
