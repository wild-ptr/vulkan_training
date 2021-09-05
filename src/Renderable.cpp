#include "Renderable.hpp"
#include "EDescriptorSets.hpp"


namespace render {
Renderable::Renderable(
        std::shared_ptr<VulkanDevice> deviceptr,
        std::shared_ptr<Pipeline> pipeline,
        std::vector<Mesh> meshes)
    : device(std::move(deviceptr))
    , meshes(std::move(meshes))
    , pipeline(std::move(pipeline))
    , uniforms(std::make_unique<memory::UniformData<RenderableUbo, consts::maxFramesInFlight>>(device))
{
    createDescriptorPool();
    generateUboDescriptorSets();
}

void Renderable::createDescriptorPool()
{
    const VkDescriptorPoolSize poolSize = {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = consts::maxFramesInFlight,
    };

    const VkDescriptorPoolCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = consts::maxFramesInFlight,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize
    };

    VK_CHECK(vkCreateDescriptorPool(device->getDevice(), &ci, nullptr, &descriptorPool));
}

void Renderable::generateUboDescriptorSets()
{
    auto setLayout = pipeline->getDescriptorSetLayout(EDescriptorSets::BindFrequency_Object);
    std::vector<VkDescriptorSetLayout> setLayouts(consts::maxFramesInFlight, setLayout);

    VkDescriptorSetAllocateInfo ai = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptorPool,
        .descriptorSetCount = consts::maxFramesInFlight,
        .pSetLayouts = setLayouts.data()
    };

    descriptorSets.resize(consts::maxFramesInFlight);

    VK_CHECK(vkAllocateDescriptorSets(device->getDevice(), &ai, descriptorSets.data()));

    auto uboBufferInfos = uniforms->getDescriptorBufferInfos();
    // we now need to fill our descriptor sets with proper buffers.
    for (size_t i = 0; i < descriptorSets.size(); ++i) {
        VkWriteDescriptorSet wds = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSets[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &uboBufferInfos[i]
        };

        vkUpdateDescriptorSets(device->getDevice(), 1, &wds, 0, nullptr);
    }
}

void Renderable::updateUniforms(RenderableUbo ubo, size_t bufferIdx)
{
    auto& uniformData = *uniforms;
    uniformData[bufferIdx] = std::move(ubo);
    uniformData.update(bufferIdx);
}

void Renderable::cmdBindSetsDrawMeshes(VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
    assert(frameIndex < descriptorSets.size());
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getHandle());

    vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline->getLayoutHandle(),
            EDescriptorSets::BindFrequency_Object, 1, // object-freq set, one set
            &descriptorSets[frameIndex],
            0, 0); // dynamic offsets junk

    for(auto& mesh : meshes)
    {
        mesh.cmdDraw(commandBuffer, pipeline->getLayoutHandle());
    }
}

} // namespace render
