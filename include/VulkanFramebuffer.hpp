#pragma once
#define GLFW_INCLUDE_VULKAN
#include "VulkanImage.hpp"
#include "VulkanSwapchain.hpp"
#include "VulkanDevice.hpp"
#include "vk_mem_alloc.h"
#include <GLFW/glfw3.h>
#include <optional>
#include <memory>

namespace render {

enum class EFramebufferAttachmentType {
    ATTACHMENT_COLOR,
    ATTACHMENT_DEPTH,
    ATTACHMENT_OFFSCREEN_ALBEDO,
    ATTACHMENT_OFFSCREEN_NORMALS,
    ATTACHMENT_OFFSCREEN_POSITION,
    ATTACHMENT_OFFSCREEN_TANGENT,
    ATTACHMENT_SWAPCHAIN_PRESENT_COLOR,
};

struct FramebufferAttachmentInfo {
    memory::VulkanImageCreateInfo ci;
    EFramebufferAttachmentType type;
};

class VulkanFramebuffer {
public:
    VulkanFramebuffer(
        std::shared_ptr<VulkanDevice> device,
        std::vector<FramebufferAttachmentInfo> ci,
        size_t numOfFramebuffers);

    // this ctor is used to create present framebuffers, one per swapchain.
    // Implicitly creates a depth attachment. Per swapchain image now but this can change in the
    // future, as all we need is a single depth attachment.
    VulkanFramebuffer(
        std::shared_ptr<VulkanDevice> device,
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
    memory::VulkanImage createDepthAttachment();
    size_t getAttachmentDescriptionIndex(EFramebufferAttachmentType type);

    struct Framebuffer {
        VkFramebuffer vkFramebuffer = VK_NULL_HANDLE;
        std::vector<memory::VulkanImage> attachments;
    };

    // same storage order as attachments vector.
    struct AttachmentInfo {
        VkAttachmentDescription description;
        EFramebufferAttachmentType type;
    };

    std::shared_ptr<VulkanDevice> device;
    std::vector<Framebuffer> framebuffers;
    std::vector<AttachmentInfo> attachmentsInfo;

    // depth handled separately for creation (for swapchain only right now), as we always only need one depth attachment.
    std::optional<memory::VulkanImage> depthAttachment;

    VkRenderPass renderPass;
    uint32_t width, height;
};

} // namespace render
