#include "VmaVulkanBuffer.hpp"
#include <cstring>

namespace render::memory {

VmaVulkanBuffer::VmaVulkanBuffer(
    std::shared_ptr<VulkanDevice> deviceptr,
    const void* data,
    size_t size,
    VkBufferUsageFlags vk_flags,
    const VmaMemoryUsage vma_usage)
    : device(std::move(deviceptr))
    , allocator(device->getVmaAllocator())
    , mapped_data(nullptr)
{
    createMemoryBuffer(size, vk_flags, vma_usage);
    copyToBuffer(data, size);
    createBufferDescriptor();
}

void VmaVulkanBuffer::copyToBuffer(
    const void* transfer_data,
    size_t size)
{
    if ((not transfer_data) or (size == 0)) {
        return;
    }

    // Truncate to buffer size. We cannot overflow into unallocated space.
    if (size > allocation_info.size) {
        dbgE << "Tring to copy more memory into buffer than allocated. Truncating to size." << NEWL;
        size = allocation_info.size;
    }

    // function preserves
    bool was_previously_mapped = mapped_data;
    memcpy(mem(), transfer_data, size);

    // If host coherency hasn't been requested, do a manual flush to make writes visible
    if ((allocated_memory_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) {
        // vmaFlushAllocation wants offset and size relative to allocation, not to
        // whole allocated memory.
        vmaFlushAllocation(allocator, allocation, 0, size);
    }

    if (not was_previously_mapped) {
        unmap();
    }
}

// does not check *data size, care
void VmaVulkanBuffer::copyToBuffer(const void* data, Offset offset_packed, Size size_packed)
{
    auto& offset = offset_packed.offset;
    auto& size = size_packed.size;

    if ((not data) or (size == 0)) {
        return;
    }

    bool was_previously_mapped = mapped_data;
    std::byte* mapped_mem = static_cast<std::byte*>(mem());

    // out-of-bounds check
    if (mapped_mem + allocation_info.size < mapped_mem + offset + size) {
        throw std::runtime_error("Trying to write out of bounds to buffer with offset and size");
    }

    memcpy(mapped_mem + offset, data, size);

    if ((allocated_memory_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) {
        vmaFlushAllocation(allocator, allocation, 0, size);
    }

    if (not was_previously_mapped) {
        unmap();
    }
}

void VmaVulkanBuffer::createMemoryBuffer(
    size_t size,
    VkBufferUsageFlags vk_flags,
    VmaMemoryUsage vma_usage)
{
    //allocate vertex buffer
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = vk_flags;

    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage = vma_usage;

    //allocate the buffer and properly bind its memory as well.
    VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaallocInfo,
        &vkBuffer,
        &allocation,
        &allocation_info));

    getPhysicalMemoryAllocInfo();
}

void VmaVulkanBuffer::createBufferDescriptor()
{
    descriptor.range = allocation_info.size;
    descriptor.offset = allocation_info.offset;
    descriptor.buffer = vkBuffer;
}

void VmaVulkanBuffer::map()
{
    if (mapped_data) {
        return;
    }

    VK_CHECK(vmaMapMemory(allocator, getVmaAllocation(), &mapped_data));
}

void VmaVulkanBuffer::unmap()
{
    if (not mapped_data) {
        return;
    }

    vmaUnmapMemory(allocator, getVmaAllocation());
    mapped_data = nullptr; // avoid dangling pointers that are no longer mapped.
}

void* VmaVulkanBuffer::mem()
{
    if (not mapped_data) {
        map();
    }

    return mapped_data;
}

void VmaVulkanBuffer::getPhysicalMemoryAllocInfo()
{
    VmaAllocatorInfo vmaallocator_info;
    vmaGetAllocatorInfo(allocator, &vmaallocator_info);

    allocated_memory_properties = deviceUtils::getMemoryProperties(
            vmaallocator_info.physicalDevice, allocation_info.memoryType);
}

} // namespace render::memory
