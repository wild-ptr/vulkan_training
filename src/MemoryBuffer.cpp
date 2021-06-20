#include "MemoryBuffer.hpp"
#include "VulkanMacros.hpp"
#include <cstring>

namespace
{
VkDescriptorBufferInfo setupDescriptor(
    VkBuffer buffer, VkDeviceSize size, VkDeviceSize offset)
{
    return [buffer, size, offset]()
    {
        VkDescriptorBufferInfo descriptor{};
        descriptor.buffer = buffer;
        descriptor.range = size;
        descriptor.offset = offset;

        return descriptor;
    }();
}

} // anonymous namespace

namespace render::mem
{

// Allocate new memory buffer.
MemoryBuffer::MemoryBuffer(
    VulkanDevice& device,
    VkBufferUsageFlags usageFlags,
    VkMemoryPropertyFlags memFlags,
    VkDeviceSize size,
    void* data)
    : device(device)
    , size(size)
{
    // create buffer, allocate memory
    VK_CHECK(device.createVkBuffer(usageFlags, size, &buffer));
    VK_CHECK(device.allocateVkDeviceMemory(memFlags, buffer, &memory, &alignment));

    // if there is data to be copied to buffer, we can copy it before binding.
    if(data)
    {
        void* mapped;
        VK_CHECK(vkMapMemory(device.getDevice(), memory, 0, size, 0, &mapped));
        memcpy(mapped, data, size);

        // we need additional steps for flushing non-coherent host memory.
        if ((memFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
        {
            const auto mappedRange = [this]()
            {
                VkMappedMemoryRange mmr{};
                mmr.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
                mmr.memory = memory;
                mmr.offset = 0;
                mmr.size = this->size;

                return mmr;
            }();
            vkFlushMappedMemoryRanges(device.getDevice(), 1, &mappedRange);
        }
        vkUnmapMemory(device.getDevice(), memory);
    }

    VK_CHECK(vkBindBufferMemory(device.getDevice(), buffer, memory, 0));

    // set descriptor for later use in DescriptorSet
    descriptor = setupDescriptor(buffer, size, 0);
}

} // namespace render::mem
