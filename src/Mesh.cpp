#include "Mesh.hpp"
#include "VulkanMacros.hpp"
#include <cstring>

namespace render {

Mesh::Mesh(std::shared_ptr<VulkanDevice> deviceptr,
        const std::vector<Vertex>& mesh_data,
        const std::vector<uint32_t>& indices,
        MeshPushConstantData data)
    : device(std::move(deviceptr))
    , allocator(device->getVmaAllocator())
    , vertices(mesh_data.size())
    , indices(indices.size())
    , vertex_buffer(device, mesh_data,
          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY)
    , index_buffer(device, indices,
          VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY)
    , push_constant_data(std::move(data))
{
}

Mesh::~Mesh()
{
    // later as i will also need move assignment and move ctor, too lazy to write now
    //if(allocator)
    //    vmaDestroyBuffer(allocator, vertex_buffer.memory_buffer, vertex_buffer.allocation);
}

void Mesh::cmdDraw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout)
{
    // we do not use getpOffset as this gives offset in underlying VkDeviceMemory.
    // And we want to go from start of the buffer, so just 0.
    VkDeviceSize size{0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, getVkInfo().getpVkBuffer(), &size);
    vkCmdBindIndexBuffer(commandBuffer, index_buffer.getVkBuffer(), 0, VK_INDEX_TYPE_UINT32);

    if(pipelineLayout != VK_NULL_HANDLE)
    {
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(MeshPushConstantData), &push_constant_data);
    }

    vkCmdDrawIndexed(commandBuffer, indices, 1, 0, 0, 0);
}
} // namespace render
