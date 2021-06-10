#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "vk_mem_alloc.h"

#include "Vertex.hpp"
#include <vector>

namespace render
{

struct AllocatedBuffer
{
    VkBuffer memory_buffer;
    VmaAllocation allocation;
};

class Mesh
{
public:
    Mesh(const std::vector<Vertex>& mesh_data, VmaAllocator);

    Mesh(std::vector<Vertex>&& mesh_data, VmaAllocator);
    Mesh(){};
    ~Mesh();

    const AllocatedBuffer& getBufferInfo() const { return vertex_buffer; }
    size_t size() const { return vertices; }

private:
    VmaAllocator allocator = nullptr;
    AllocatedBuffer vertex_buffer{};
    size_t vertices{};
    // As for buffers, every mesh now will use new VkBuffer. This is explicitly retarded,
    // and every Vulkan book will tell you not to do this. I wont do this as well, in the future.
};

} // namespace render
