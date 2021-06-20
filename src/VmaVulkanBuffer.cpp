#include <cstring>
#include "VmaVulkanBuffer.hpp"
#include "VulkanDevice.hpp"


namespace render::memory
{

VmaVulkanBuffer::VmaVulkanBuffer(
    VmaAllocator allocator,
    const void* data,
    size_t size,
    VkBufferUsageFlags vk_flags,
    VmaMemoryUsage vma_usage)
    : allocator(allocator)
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
    if((not transfer_data) or (size == 0))
    {
        return;
    }

    // Truncate to buffer size. We cannot overflow into unallocated space.
    if(size > allocation_info.size)
    {
        dbgE << "Tring to copy more memory into buffer than allocated. Truncating to size." << NEWL;
        size = allocation_info.size;
    }

    bool was_previously_mapped = mapped_data;
	memcpy(mem(), transfer_data, size);

	// If host coherency hasn't been requested, do a manual flush to make writes visible
	if ((allocated_memory_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
	{
        // vmaFlushAllocation wants offset and size relative to allocation, not to
        // whole allocated memory.
		vmaFlushAllocation(allocator, allocation, 0, size);
	}

    if(not was_previously_mapped)
    {
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
    if(mapped_data)
    {
        return;
    }

    VK_CHECK(vmaMapMemory(allocator, getVmaAllocation(), &mapped_data));
}

void VmaVulkanBuffer::unmap()
{
    if(not mapped_data)
    {
        return;
    }

    vmaUnmapMemory(allocator, getVmaAllocation());
    mapped_data = nullptr; // avoid dangling pointers that are no longer mapped.
}

void* VmaVulkanBuffer::mem()
{
    if(not mapped_data)
    {
        map();
    }

    return mapped_data;
}

void VmaVulkanBuffer::getPhysicalMemoryAllocInfo()
{
    VmaAllocatorInfo vmaallocator_info;
    vmaGetAllocatorInfo(allocator, &vmaallocator_info);

    allocated_memory_properties =
        deviceUtils::getMemoryProperties(vmaallocator_info.physicalDevice, allocation_info.memoryType);
}

} // namespace render::memory