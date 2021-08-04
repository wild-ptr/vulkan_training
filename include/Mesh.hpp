#pragma once
#define GLFW_INCLUDE_VULKAN
#include "vk_mem_alloc.h"
#include <GLFW/glfw3.h>

#include "Vertex.hpp"
#include "VulkanDevice.hpp"
#include "VmaVulkanBuffer.hpp"
#include <vector>

namespace render {

class Mesh {
public:
    Mesh(const std::vector<Vertex>& mesh_data, std::shared_ptr<VulkanDevice> device);
    Mesh(const std::vector<Vertex>&& mesh_data, std::shared_ptr<VulkanDevice> device);
    Mesh() {};
    ~Mesh();

    const memory::VmaVulkanBuffer& getVkInfo() const { return vertex_buffer; }
    size_t vertexCount() const { return vertices; }
    void cmdDraw(VkCommandBuffer);

private:
    std::shared_ptr<VulkanDevice> device;
    VmaAllocator allocator;
    size_t vertices;
    memory::VmaVulkanBuffer vertex_buffer;
};

} // namespace render
