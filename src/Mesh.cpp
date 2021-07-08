#include "Mesh.hpp"
#include "VulkanMacros.hpp"
#include <cstring>

namespace {

} // anon namespace

namespace render {

Mesh::Mesh(const std::vector<Vertex>& mesh_data, VmaAllocator allocator)
    : allocator(allocator)
    , vertices(mesh_data.size())
    , vertex_buffer(allocator, mesh_data,
          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU)
{
}

Mesh::Mesh(std::vector<Vertex>&& mesh_data, VmaAllocator allocator)
    : allocator(allocator)
    , vertices(mesh_data.size())
    , vertex_buffer(allocator, mesh_data,
          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU)
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
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, getVkInfo().getpVkBuffer(), getVkInfo().getpOffset());
    vkCmdDraw(commandBuffer, vertexCount(), 1, 0, 0);
}
} // namespace render
