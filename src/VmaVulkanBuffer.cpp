#include "VmaVulkanBuffer.hpp"
#include <cstring>
#include <cassert>

namespace render::memory {

VmaVulkanBuffer::VmaVulkanBuffer(
    std::shared_ptr<VulkanDevice> deviceptr,
    const void* data,
    size_t size,
    VkBufferUsageFlags vk_flags,
    const VmaMemoryUsage vma_usage)
    : device(std::move(deviceptr))
    , allocator(device->getVmaAllocator())
    , is_gpu_buffer(vma_usage == VMA_MEMORY_USAGE_GPU_ONLY)
{
    if(is_gpu_buffer)
    {
        buffer = createMemoryBuffer(
                size,
                vk_flags | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                vma_usage);

        auto stagingBuffer = createMemoryBuffer(
                size,
                vk_flags | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VMA_MEMORY_USAGE_CPU_ONLY);

        copyToBuffer(stagingBuffer, data, size);

        device->immediateSubmitBlocking(
                [&](VkCommandBuffer cmd)
                {
                    VkBufferCopy copy = {
		                .srcOffset = 0,
		                .dstOffset = 0,
		                .size = size,
                    };

		            vkCmdCopyBuffer(cmd, stagingBuffer.vkBuffer, buffer.vkBuffer, 1, &copy);
                });

        vmaDestroyBuffer(stagingBuffer.allocator, stagingBuffer.vkBuffer, stagingBuffer.allocation);
    }
    else
    {
        buffer = createMemoryBuffer(size, vk_flags, vma_usage);
        copyToBuffer(buffer, data, size);
    }
}

void VmaVulkanBuffer::copyToBuffer(
    BufferInfo& buffer,
    const void* transfer_data,
    size_t size)
{
    if ((not transfer_data) or (size == 0)) {
        return;
    }

    // Truncate to buffer size. We cannot overflow into unallocated space.
    if (size > buffer.allocation_info.size) {
        dbgE << "Trying to copy more memory into buffer than allocated. Truncating to size." << NEWL;
        size = buffer.allocation_info.size;
    }

    // function preserves map state
    bool was_previously_mapped = buffer.mapped_data;
    memcpy(buffer.mem(), transfer_data, size);

    // If host coherency hasn't been requested, do a manual flush to make writes visible
    if ((buffer.allocated_memory_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) {
        // vmaFlushAllocation wants offset and size relative to allocation, not to
        // whole allocated memory.
        vmaFlushAllocation(buffer.allocator, buffer.allocation, 0, size);
    }

    if (not was_previously_mapped) {
        buffer.unmap();
    }
}

void VmaVulkanBuffer::copyToBuffer(
    const void* transfer_data,
    size_t size)
{
    if(is_gpu_buffer)
    {
        assert(false); // not implemented
    }
    copyToBuffer(buffer, transfer_data, size);
}

// does not check *data size, care
void VmaVulkanBuffer::copyToBuffer(BufferInfo& buffer, const void* data,
        Offset offset_packed, Size size_packed)
{
    if(is_gpu_buffer)
    {
        assert(false); // not implemented
    }

    auto offset = offset_packed.offset;
    auto size = size_packed.size;

    if ((not data) or (size == 0)) {
        return;
    }

    bool was_previously_mapped = buffer.mapped_data;
    std::byte* mapped_mem = static_cast<std::byte*>(buffer.mem());

    // out-of-bounds check
    if (mapped_mem + buffer.allocation_info.size < mapped_mem + offset + size) {
        throw std::runtime_error("Trying to write out of bounds to buffer with offset and size");
    }

    memcpy(mapped_mem + offset, data, size);

    if ((buffer.allocated_memory_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) {
        vmaFlushAllocation(buffer.allocator, buffer.allocation, 0, size);
    }

    if (not was_previously_mapped) {
        buffer.unmap();
    }
}

// does not check *data size, care
void VmaVulkanBuffer::copyToBuffer(const void* data, Offset offset_packed, Size size_packed)
{
    copyToBuffer(buffer, data, offset_packed, size_packed);
}

VmaVulkanBuffer::BufferInfo VmaVulkanBuffer::createMemoryBuffer(
    size_t size,
    VkBufferUsageFlags vk_flags,
    VmaMemoryUsage vma_usage)
{
    //allocate vertex buffer
    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = vk_flags,
    };

    VmaAllocationCreateInfo vmaallocInfo = {
        .usage = vma_usage
    };

    BufferInfo info{};
    //allocate the buffer and properly bind its memory as well.
    VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaallocInfo,
        &info.vkBuffer,
        &info.allocation,
        &info.allocation_info));

    getPhysicalMemoryAllocInfo(info);
    info.allocator = allocator;
    createBufferDescriptor(info);

    return info;
}

void VmaVulkanBuffer::createBufferDescriptor(BufferInfo& info)
{
    info.descriptor.range = info.allocation_info.size;
    info.descriptor.offset = info.allocation_info.offset;
    info.descriptor.buffer = info.vkBuffer;
}

void VmaVulkanBuffer::map()
{
    buffer.map();
}

void VmaVulkanBuffer::unmap()
{
    buffer.unmap();
}

void* VmaVulkanBuffer::mem()
{
    return buffer.mem();
}

void VmaVulkanBuffer::getPhysicalMemoryAllocInfo(BufferInfo& bufferInfo)
{
    VmaAllocatorInfo vmaallocator_info;
    vmaGetAllocatorInfo(allocator, &vmaallocator_info);

    bufferInfo.allocated_memory_properties = deviceUtils::getMemoryProperties(
            vmaallocator_info.physicalDevice, bufferInfo.allocation_info.memoryType);
}

} // namespace render::memory
