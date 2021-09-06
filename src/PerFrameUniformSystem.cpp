#include "PerFrameUniformSystem.hpp"

namespace render::memory
{
PerFrameUniformSystem::PerFrameUniformSystem(
        std::shared_ptr<VulkanDevice> device_ptr,
        std::shared_ptr<TextureManager> texmgr_ptr,
        std::shared_ptr<CameraSystem> camerasys_ptr,
        std::shared_ptr<Pipeline> pipeline_ptr)
    : device(std::move(device_ptr))
    , texture_mgr(std::move(texmgr_ptr))
    , camera(std::move(camerasys_ptr))
    , pipeline(std::move(pipeline_ptr))
    , ubo(std::make_unique<UniformData<PerFrameUbo, consts::maxFramesInFlight>>(device))
{
    createDescriptorPool();
    generateDescriptorSets();
    fillDescriptorSets();
    fillUboDescriptor();
}

void PerFrameUniformSystem::createDescriptorPool()
{
    auto poolSizes = texture_mgr->getBindingInformation().poolSizes;

    // @TODO: this should be in UniformData, as it spills implementation details.
    // And is generally awful.
    const VkDescriptorPoolSize uniformBufferPoolSizes = {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = consts::maxFramesInFlight,
    };

    poolSizes.emplace_back(std::move(uniformBufferPoolSizes));

    const VkDescriptorPoolCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = consts::maxFramesInFlight,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data()
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

void PerFrameUniformSystem::fillUboDescriptor()
{
    auto uniformBufferInfos = ubo->getDescriptorBufferInfos();

    assert(uniformBufferInfos.size() == descriptorSets.size());

    for(size_t i = 0; i < descriptorSets.size(); ++i)
    {
        VkWriteDescriptorSet uboBinding = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSets[i],
            .dstBinding = consts::perFrame_uboBinding,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &uniformBufferInfos[i]
        };

        vkUpdateDescriptorSets(device->getDevice(), 1, &uboBinding, 0, nullptr);
    }
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

    // this is extremely wasteful, do an observer or something later. Yuck.
    texture_mgr->fillDescriptorSet(descriptorSets[setIdx]);

    (*ubo)[setIdx].camera = camera->genCurrentVPMatrices();
    ubo->update(setIdx);
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
