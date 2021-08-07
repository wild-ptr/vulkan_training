#include "Mesh.hpp"
#include "VulkanMacros.hpp"
#include <cstring>

namespace render {

Mesh::Mesh(const std::vector<Vertex>& mesh_data, std::shared_ptr<VulkanDevice> deviceptr)
    : device(std::move(deviceptr))
    , allocator(device->getVmaAllocator())
    , vertices(mesh_data.size())
    , vertex_buffer(device, mesh_data,
          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY)
{
}

Mesh::Mesh(const std::vector<Vertex>&& mesh_data, std::shared_ptr<VulkanDevice> deviceptr)
    : device(std::move(deviceptr))
    , allocator(device->getVmaAllocator())
    , vertices(mesh_data.size())
    , vertex_buffer(device, mesh_data,
          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY)
{
}

Mesh::~Mesh()
{
    // later as i will also need move assignment and move ctor, too lazy to write now
    //if(allocator)
    //    vmaDestroyBuffer(allocator, vertex_buffer.memory_buffer, vertex_buffer.allocation);
}

void Mesh::cmdDraw(VkCommandBuffer commandBuffer)
{
    // we do not use getpOffset as this gives offset in underlying VkDeviceMemory.
    // And we want to go from start of the buffer, so just 0.
    VkDeviceSize size{0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, getVkInfo().getpVkBuffer(), &size);
    vkCmdDraw(commandBuffer, vertexCount(), 1, 0, 0);
}
} // namespace render
