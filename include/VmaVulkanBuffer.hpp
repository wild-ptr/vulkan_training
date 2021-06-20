#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "vk_mem_alloc.h"
#include <vector>
#include <cstring>
#include "Vertex.hpp"
#include "VulkanMacros.hpp"

namespace render::memory
{

class VmaVulkanBuffer
{
public:
    // ctor from void* arbitrary data
    VmaVulkanBuffer(
        VmaAllocator allocator,
        const void* data,
        size_t size,
        VkBufferUsageFlags vk_flags,
        VmaMemoryUsage vma_usage);

    // ctor from std::vector<T> data
    template<typename T>
    VmaVulkanBuffer(
        VmaAllocator allocator,
        const std::vector<T>& data,
        VkBufferUsageFlags vk_flags,
        VmaMemoryUsage vma_usage)
    : allocator(allocator)
    , mapped_data(nullptr)
    {
        createMemoryBuffer(data.size() * sizeof(T), vk_flags, vma_usage);
        copyToBuffer(data.data(), data.size() * sizeof(T));
        createBufferDescriptor();
    }

    VmaVulkanBuffer(){};
    ~VmaVulkanBuffer(){};

    const VkBuffer* getpVkBuffer() const { return &vkBuffer; }
    const VkDeviceSize* getpOffset() const { return &allocation_info.offset; }
    VkDeviceSize getSizeBytes() const { return allocation_info.size; }
    VkDeviceMemory getVkDeviceMemory() const { return allocation_info.deviceMemory; }
    VmaAllocation getVmaAllocation() const { return allocation; }
    const VkDescriptorBufferInfo& getDescriptor() const { return descriptor; }

    void map();
    void unmap();
    void* mem();

    // copyToBuffer functions will truncate to buffer size if size is higher.
    void copyToBuffer(const void* data, size_t size);


private:
    void createMemoryBuffer(
        size_t size,
        VkBufferUsageFlags vk_flags,
        VmaMemoryUsage vma_usage);

    void createBufferDescriptor();
    void getPhysicalMemoryAllocInfo();

    VmaAllocator allocator;
    void* mapped_data;
    VkBuffer vkBuffer;
    VkDescriptorBufferInfo descriptor;
    VmaAllocation allocation;
    VmaAllocationInfo allocation_info;
    VkMemoryPropertyFlags allocated_memory_properties;
};

} // namespace render::memory