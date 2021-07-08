#include "Renderable.hpp"

namespace render {
Renderable::Renderable(const VulkanDevice& device, std::vector<Mesh> meshes)
    : device(device)
    , meshes(std::move(meshes))
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

    VK_CHECK(vkCreateDescriptorPool(device.getDevice(), &ci, nullptr, &descriptorPool));
}

void Renderable::generateUboDescriptorSets()
{
    // first indice is going to be our per-object set for now.
    // I need some better handling of this as i hate such magic numbers.
    // Enumerated set number - name?
    auto setLayout = pipeline->getSetLayoutIndex(1);
    std::vector<VkDescriptorSetLayout> setLayouts(consts::maxFramesInFlight, setLayout);

    VkDescriptorSetAllocateInfo ai = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptorPool,
        .descriptorSetCount = consts::maxFramesInFlight,
        .pSetLayouts = setLayouts.data()
    };

    descriptorSets.resize(consts::maxFramesInFlight);

    VK_CHECK(vkAllocateDescriptorSets(device.getDevice(), &ai, descriptorSets.data()));

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

        vkUpdateDescriptorSets(device.getDevice(), 1, &wds, 0, nullptr);
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
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getHandle());

    for(auto& mesh : meshes)
    {
        mesh.cmdDraw(commandBuffer);
    }

    vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline->getLayoutHandle(),
            1, 1, // first set, one set
            &descriptorSets[frameIndex],
            0, 0); // dynamic offsets junk
}

} // namespace render
