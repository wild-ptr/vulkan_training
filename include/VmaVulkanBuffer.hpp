#pragma once
#define GLFW_INCLUDE_VULKAN
#include "Vertex.hpp"
#include "VulkanDevice.hpp"
#include "VulkanMacros.hpp"
#include "vk_mem_alloc.h"
#include <GLFW/glfw3.h>
#include <cstring>
#include <vector>
#include <memory>

namespace render::memory {

// explicit interface for copyToBuffer
struct Offset {
    explicit Offset(size_t offset)
        : offset(offset)
    {
    }
    size_t offset;
};

struct Size {
    explicit Size(size_t size)
        : size(size)
    {
    }
    size_t size;
};

class VmaVulkanBuffer {
    struct BufferInfo;
public:
    // ctor from void* arbitrary data
    VmaVulkanBuffer(
        std::shared_ptr<VulkanDevice> device,
        const void* data,
        size_t size,
        VkBufferUsageFlags vk_flags,
        const VmaMemoryUsage vma_usage);

    // ctor from std::vector<T> data
    template <typename T>
    VmaVulkanBuffer(
        std::shared_ptr<VulkanDevice> deviceptr,
        const std::vector<T>& data,
        VkBufferUsageFlags vk_flags,
        VmaMemoryUsage vma_usage)
    : VmaVulkanBuffer(deviceptr, data.data(), data.size() * sizeof(T), vk_flags, vma_usage)
    {
    }

    VmaVulkanBuffer() {};
    ~VmaVulkanBuffer() {};

    const VkBuffer* getpVkBuffer() const { return &buffer.vkBuffer; }
    VkBuffer getVkBuffer() const { return buffer.vkBuffer; }
    const VkDeviceSize* getpOffset() const { return &buffer.allocation_info.offset; }
    VkDeviceSize getSizeBytes() const { return buffer.allocation_info.size; }
    VkDeviceMemory getVkDeviceMemory() const { return buffer.allocation_info.deviceMemory; }
    VmaAllocation getVmaAllocation() const { return buffer.allocation; }
    const VkDescriptorBufferInfo& getDescriptor() const { return buffer.descriptor; }

    void map();
    void unmap();
    void* mem();

    // copyToBuffer functions will truncate to buffer size if size is higher.
    void copyToBuffer(const void* data, size_t size);
    // hard interface types to avoid mistaking size and offset
    void copyToBuffer(const void* data, Offset offset, Size size);

private:
    struct BufferInfo
    {
        VkBuffer vkBuffer{VK_NULL_HANDLE};
        VmaAllocation allocation;
        VmaAllocationInfo allocation_info;
        VkMemoryPropertyFlags allocated_memory_properties;
        VkDescriptorBufferInfo descriptor;
        void* mapped_data{nullptr};
        VmaAllocator allocator;

        void map()
        {
            if (mapped_data) {
                return;
            }

            VK_CHECK(vmaMapMemory(allocator, allocation, &mapped_data));
        }

        void unmap()
        {
            if (not mapped_data) {
                return;
            }

            vmaUnmapMemory(allocator, allocation);
            mapped_data = nullptr; // avoid dangling pointers that are no longer mapped.
        }

        void* mem()
        {
            if (not mapped_data) {
                map();
            }

            return mapped_data;
        }
    };

    BufferInfo createMemoryBuffer(
        size_t size,
        VkBufferUsageFlags vk_flags,
        VmaMemoryUsage vma_usage);

    void createBufferDescriptor(BufferInfo&);
    void getPhysicalMemoryAllocInfo(BufferInfo&);
    void copyToBuffer(BufferInfo&, const void* data, size_t size);
    void copyToBuffer(BufferInfo&, const void* data, Offset offset, Size size);

    std::shared_ptr<VulkanDevice> device;
    VmaAllocator allocator;
    BufferInfo buffer;
    bool is_gpu_buffer;
};

} // namespace render::memory
