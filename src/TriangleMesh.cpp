#include "TriangleMesh.hpp"
#include "VulkanMacros.hpp"
#include <cstring>

namespace
{

void copyToBuffer(
        VmaAllocator allocator,
        render::AllocatedBuffer& vertex_buffer,
        const std::vector<render::Vertex>& mesh_data)
{
   	void* data;
	vmaMapMemory(allocator, vertex_buffer.allocation, &data);

	memcpy(data, mesh_data.data(), mesh_data.size() * sizeof(render::Vertex));

	vmaUnmapMemory(allocator, vertex_buffer.allocation);
}


void createMemoryBuffer(
        VmaAllocator allocator,
        render::AllocatedBuffer& vertex_buffer,
        const std::vector<render::Vertex>& mesh_data)
{
    //allocate vertex buffer
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = mesh_data.size() * sizeof(render::Vertex);
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	//let the VMA library know that this data should be writeable by CPU, but also readable by GPU
	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

	//allocate the buffer
	VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaallocInfo,
		&vertex_buffer.memory_buffer,
		&vertex_buffer.allocation,
		nullptr),
        "Failed to create buffer for mesh!");
}

} // anon namespace

namespace render
{

Mesh::Mesh(const std::vector<Vertex>& mesh_data, VmaAllocator allocator)
    : allocator(allocator)
    , vertices(mesh_data.size())
{
    createMemoryBuffer(allocator, vertex_buffer, mesh_data);
    copyToBuffer(allocator, vertex_buffer, mesh_data);
}

Mesh::Mesh(std::vector<Vertex>&& mesh_data, VmaAllocator allocator)
    : allocator(allocator)
    , vertices(mesh_data.size())
{
    createMemoryBuffer(allocator, vertex_buffer, mesh_data);
    copyToBuffer(allocator, vertex_buffer, mesh_data);
}

Mesh::~Mesh()
{
    //if(allocator)
    //    vmaDestroyBuffer(allocator, vertex_buffer.memory_buffer, vertex_buffer.allocation);
}

} // namespace render
