#include "VulkanFramebuffer.hpp"
#include "VulkanMacros.hpp"
#include <algorithm>
#include <cassert>

namespace render {

// this creates presentation framebuffers, one per swapchain image.
// Doesnt work yet as we are getting validation layer errors about layercount from subresource range.
VulkanFramebuffer::VulkanFramebuffer(VmaAllocator allocator, const VulkanSwapchain& swapchain, bool createDepthAttachment)
    : allocator(allocator)
{
    width = swapchain.getSwapchainExtent().width;
    height = swapchain.getSwapchainExtent().height;

    // fill in attachment infos.
    AttachmentInfo information = {
        .description = {
            .format = swapchain.getSwapchainImageFormat(),
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR },
        .type = EFramebufferAttachmentType::ATTACHMENT_SWAPCHAIN_PRESENT_COLOR
    };

    attachmentsInfo.emplace_back(std::move(information));

    auto swapchainImagesSize = swapchain.size();
    const auto& swapchainImages = swapchain.getSwapchainImages();
    const auto& swapchainImageViews = swapchain.getSwapchainImageViews();
    auto format = swapchain.getSwapchainImageFormat();
    const auto& subresourceRange = swapchain.getSwapchainSubresourceRange();

    for (size_t i = 0; i < swapchainImagesSize; ++i) {
        Framebuffer fb = {
            .attachments = {
                memory::VulkanImage(
                    swapchainImages[i],
                    swapchainImageViews[i],
                    format,
                    subresourceRange) }
        };

        framebuffers.emplace_back(std::move(fb));
    }

    createRenderPass();
    createFramebuffer();
}

VulkanFramebuffer::VulkanFramebuffer(VmaAllocator allocator, std::vector<FramebufferAttachmentInfo> ci, size_t numOfFramebuffers)
    : allocator(allocator)
{
    width = ci[0].ci.width;
    height = ci[0].ci.height;

    // is this even a good assertion? Perhaps there are cases where i want framebuffer
    // attachments of different size? Shadowmaps? :/
    assert(std::all_of(std::begin(ci), std::end(ci),
        [this](const auto& info) {
            return info.ci.width == width and info.ci.height == height;
        })); // ill keep this untill i need different sizes.

    for (auto& attachmentInfo : ci) {
        auto image = memory::VulkanImage(attachmentInfo.ci, allocator);

        AttachmentInfo information = {
            .description = {
                .format = attachmentInfo.ci.format,
                .samples = attachmentInfo.ci.imageSampleCount,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = (attachmentInfo.ci.usage & VK_IMAGE_USAGE_SAMPLED_BIT) ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = image.hasDepthOrStencil() ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
            .type = attachmentInfo.type
        };

        attachmentsInfo.emplace_back(std::move(information));
    }

    for (size_t framebufferIdx = 0; framebufferIdx < numOfFramebuffers; ++framebufferIdx) {
        Framebuffer framebuffer {};
        for (auto& attachmentInfo : ci) {
            auto image = memory::VulkanImage(attachmentInfo.ci, allocator);
            framebuffer.attachments.emplace_back(std::move(image));
        }
        framebuffers.emplace_back(framebuffer);
    }

    // after the Framebuffer vector is ready and we have all attachments in check, we can create a renderpass based on that,
    // and then finally the VkFramebuffer objects.
    createRenderPass();
    createFramebuffer();
}

void VulkanFramebuffer::createRenderPass()
{
    std::vector<VkAttachmentDescription> attachmentDescriptions;

    for (auto& attachment : attachmentsInfo) {
        attachmentDescriptions.push_back(attachment.description);
    }

    // Collect attachment references
    std::vector<VkAttachmentReference> colorReferences;
    VkAttachmentReference depthReference = {};
    bool hasDepth = false;
    bool hasColor = false;

    uint32_t attachmentIndex = 0;

    auto& firstFramebufImages = framebuffers[0].attachments;
    for (auto& attachment : firstFramebufImages) {
        if (attachment.hasDepthOrStencil()) {
            // Only one depth attachment allowed
            assert(!hasDepth);
            depthReference.attachment = attachmentIndex;
            depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            hasDepth = true;
        } else {
            colorReferences.push_back(
                { .attachment = attachmentIndex,
                    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
            hasColor = true;
        }

        attachmentIndex++;
    };

    const auto subpass = [hasColor, hasDepth, &colorReferences, &depthReference]() {
        VkSubpassDescription s {};
        s.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        if (hasColor) {
            s.pColorAttachments = colorReferences.data();
            s.colorAttachmentCount = colorReferences.size();
        }
        if (hasDepth) {
            s.pDepthStencilAttachment = &depthReference;
        }

        return s;
    }();

    //std::array<VkSubpassDependency, 2> dependencies;

    // i am unsure about dependencies, will probably do it a bit differently.
    //dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    //dependencies[0].dstSubpass = 0;
    //dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    //dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    //dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    //dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
    //                                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    //dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    //
    //dependencies[1].srcSubpass = 0;
    //dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    //dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    //dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    //dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
    //                                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    //dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    //dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkSubpassDependency sd {};
    sd.srcSubpass = VK_SUBPASS_EXTERNAL; // implicit subpass before dstSubpass.
    sd.dstSubpass = 0; // index of subpass

    // The renderpass has two implicit stages, one at the end of the render pass
    // to transition image into presentation layout, and one at the beginning to
    // transition newly acquired image to beginning layout.
    // The renderpass will do this in the very moment we start our pipeline,
    // and this is too early, as we still havent acquired an image at this point.
    // So we tell renderpass to wait with memory transitions untill we get to fragment shader,
    // as then (as we specified with semaphores during submission) image must be available to us.
    // src - wait on this stage to happen
    sd.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    sd.srcAccessMask = 0;

    // Color attachment write operation in color attachment stage
    // should wait for transition before writing.
    // what should wait on src - in our case attachment writing should wait.
    sd.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    sd.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // Create render pass
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.pAttachments = attachmentDescriptions.data();
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &sd;
    //renderPassInfo.dependencyCount = 2;
    //renderPassInfo.pDependencies = dependencies.data();

    VmaAllocatorInfo allocInfo;
    vmaGetAllocatorInfo(allocator, &allocInfo);

    VK_CHECK(vkCreateRenderPass(allocInfo.device, &renderPassInfo, nullptr, &renderPass));
}

void VulkanFramebuffer::createFramebuffer()
{
    for (size_t i = 0; i < framebuffers.size(); ++i) {
        std::vector<VkImageView> attachmentViews;

        auto& framebufferStruct = framebuffers[i];
        for (auto& image : framebufferStruct.attachments) {
            attachmentViews.push_back(image.getImageView());
        }

        // we need to find maximum layer count from all attachments.
        auto it = std::max_element(
            std::begin(framebufferStruct.attachments),
            std::end(framebufferStruct.attachments),
            [](auto& a, auto& b) {
                return a.getSubresourceRange().layerCount < b.getSubresourceRange().layerCount;
            });
        assert(it != std::end(framebufferStruct.attachments));
        uint32_t maxLayers = it->getSubresourceRange().layerCount;

        const auto framebufferCi = [this, &attachmentViews, &maxLayers]() {
            VkFramebufferCreateInfo ci {};
            ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            ci.renderPass = renderPass;
            ci.pAttachments = attachmentViews.data();
            ci.attachmentCount = attachmentViews.size();
            ci.width = width;
            ci.height = height;
            ci.layers = maxLayers;

            return ci;
        }();

        VmaAllocatorInfo allocInfo;
        vmaGetAllocatorInfo(allocator, &allocInfo);
        VK_CHECK(vkCreateFramebuffer(allocInfo.device, &framebufferCi, nullptr, &framebufferStruct.vkFramebuffer));
    }
}

VkAttachmentDescription VulkanFramebuffer::getAttachmentDescription(EFramebufferAttachmentType type)
{
    assert(framebuffers.size() != 0);

    auto it = std::find_if(std::begin(attachmentsInfo), std::end(attachmentsInfo),
        [type](const AttachmentInfo& attachment) {
            return attachment.type == type;
        });

    assert(it != std::end(attachmentsInfo));

    return it->description;
}

} // namespace render
