#pragma once
#define GLFW_INCLUDE_VULKAN
#include "vk_mem_alloc.h"
#include <GLFW/glfw3.h>

#include "Vertex.hpp"
#include "VulkanDevice.hpp"
#include "VmaVulkanBuffer.hpp"
#include <vector>

namespace render {

// blinn-phong model for now.
struct MeshPushConstantData
{
    uint32_t diffuse_texid;
    uint32_t normal_texid;
    uint32_t specular_texid;
};

class Mesh {
public:
    Mesh(std::shared_ptr<VulkanDevice> dev,
         const std::vector<Vertex>& mesh_data,
         const std::vector<uint32_t>& indices,
         MeshPushConstantData data);

    Mesh() {};
    ~Mesh();

    const memory::VmaVulkanBuffer& getVkInfo() const { return vertex_buffer; }
    const memory::VmaVulkanBuffer& getVkInfoIndexBuffer() const { return index_buffer; }
    size_t vertexCount() const { return vertices; }
    void cmdDraw(VkCommandBuffer, VkPipelineLayout);

private:
    std::shared_ptr<VulkanDevice> device;
    VmaAllocator allocator;
    size_t vertices;
    size_t indices;
    memory::VmaVulkanBuffer vertex_buffer;
    memory::VmaVulkanBuffer index_buffer;
    MeshPushConstantData push_constant_data;
};

} // namespace render
