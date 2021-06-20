#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VulkanDevice.hpp"

namespace render::mem
{

// This class does things without Vulkan Memory Allocator library.
// Right now it creates new memory buffers every time, instead of subdividing
// preallocated memory as things should be. todo i guess.
class MemoryBuffer
{
    MemoryBuffer(
        VulkanDevice& device,
        VkBufferUsageFlags usageFlags,
        VkMemoryPropertyFlags flags,
        VkDeviceSize size,
        void* data = nullptr);

    // use already-created memory for the buffer.
    MemoryBuffer(VkDevice device, VkDeviceMemory memory);

    void* mem();
    VkResult unmap();
    VkResult map();
    bool copyTo(void* data, VkDeviceSize size);
    bool copyTo(MemoryBuffer& buffer);
    const VkDescriptorBufferInfo& getDescriptor() { return descriptor; }

private:
    VulkanDevice& device;
    VkDeviceSize size;
    VkDeviceSize alignment;
    VkDeviceMemory memory;
    VkBuffer buffer;
    VkDescriptorBufferInfo descriptor{};

};

} // namespace render::mem
