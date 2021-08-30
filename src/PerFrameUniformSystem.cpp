#include "PerFrameUniformSystem.hpp"

namespace render::memory
{
PerFrameUniformSystem::PerFrameUniformSystem(
        std::shared_ptr<VulkanDevice> device_ptr,
        std::shared_ptr<TextureManager> texmgr_ptr,
        std::shared_ptr<Pipeline> pipeline_ptr)
    : device(std::move(device_ptr))
    , texture_mgr(std::move(texmgr_ptr))
    , pipeline(std::move(pipeline_ptr))
{
    createDescriptorPool();
    generateDescriptorSets();
    fillDescriptorSets();
}

void PerFrameUniformSystem::createDescriptorPool()
{
    // other pool sizes will follow later.
    auto texturePoolSizes = texture_mgr->getBindingInformation().poolSizes;

    const VkDescriptorPoolCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = consts::maxFramesInFlight,
        .poolSizeCount = static_cast<uint32_t>(texturePoolSizes.size()),
        .pPoolSizes = texturePoolSizes.data()
    };

    VK_CHECK(vkCreateDescriptorPool(device->getDevice(), &ci, nullptr, &descriptorPool));
}

void PerFrameUniformSystem::generateDescriptorSets()
{
    auto setLayout = pipeline->getDescriptorSetLayout(EDescriptorSets::BindFrequency_Frame);
    std::vector<VkDescriptorSetLayout> setLayouts(consts::maxFramesInFlight, setLayout);

    VkDescriptorSetAllocateInfo ai =
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptorPool,
        .descriptorSetCount = consts::maxFramesInFlight,
        .pSetLayouts = setLayouts.data()
    };

    descriptorSets.resize(consts::maxFramesInFlight);
    VK_CHECK(vkAllocateDescriptorSets(device->getDevice(), &ai, descriptorSets.data()));
}


void PerFrameUniformSystem::fillDescriptorSets()
{
    for(auto set : descriptorSets)
    {
        texture_mgr->fillDescriptorSet(set);
    };
}

void PerFrameUniformSystem::refreshData(uint32_t setIdx)
{
    assert(setIdx < descriptorSets.size() and setIdx < consts::maxFramesInFlight);

    texture_mgr->fillDescriptorSet(descriptorSets[setIdx]);
}

void PerFrameUniformSystem::bind(VkCommandBuffer buf, uint32_t setIdx)
{
    assert(setIdx < descriptorSets.size());

    vkCmdBindDescriptorSets(
            buf,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline->getLayoutHandle(),
            EDescriptorSets::BindFrequency_Frame, 1, // one set, the object freq set
            &descriptorSets[setIdx],
            0, 0);
}

} // namespace render
