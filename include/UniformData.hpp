#pragma once
#define GLFW_INCLUDE_VULKAN
#include "VmaVulkanBuffer.hpp"
#include <GLFW/glfw3.h>
#include <array>

namespace render::memory {

// Wrapper class for uniform data. Treat as standard singular uniform (or array of them)
// .update()
// to make changes gpu-visible. Proper synchronization is still required.
// Can be used to create one buffer that will hold framesInFlight UBO's
// Will generate its own descriptorSet, or descriptorBufferInfo, depending on needs.


// free functions as they are generic enough that others might want to reuse them.
inline VkDeviceSize getMinUboOffsetAlignment(VmaAllocator allocator)
{
    VmaAllocatorInfo vmaallocator_info;
    vmaGetAllocatorInfo(allocator, &vmaallocator_info);
    auto& gpu = vmaallocator_info.physicalDevice;

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(gpu, &properties);

    return properties.limits.minUniformBufferOffsetAlignment;
}

template<typename T>
size_t calcAlignedTypeSize(size_t alignment)
{
    // lets say T is 64 bytes, and wanted alignment is 40 bytes.
    // so 64/40 = 1, mow we check the modulo, and if its non-zero, we add one alignment in bytes.
    // otherwise T is multiply of alignment amd we do not need padding.
    // so 64 -> 80, 120 -> 120, 12 -> 40 for required alignment of 40.
    size_t wholes = sizeof(T) / alignment;
    size_t mod = sizeof(T) % alignment;

    if(mod > 0)
    {
        return ((wholes * alignment) + alignment);
    }

    return sizeof(T);
}

template <class T, unsigned N = 1>
class UniformData {
public:
    UniformData(VmaAllocator allocator)
        : aligned_t_size(calcAlignedTypeSize<T>(getMinUboOffsetAlignment(allocator)))
        , ubo_buffer(allocator, nullptr, aligned_t_size * N,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, // buffer usage flags
            VMA_MEMORY_USAGE_CPU_TO_GPU) // memory usage flags
    {
        // shall be permanently mapped, as uniforms change often.
        ubo_buffer.map();
    }

    UniformData(VmaAllocator allocator, std::array<T, N> data)
        : ubo_arr(std::move(data))
        , aligned_t_size(calcAlignedTypeSize<T>(getMinUboOffsetAlignment(allocator)))
        , ubo_buffer(allocator, nullptr, aligned_t_size * N,
              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, // buffer usage flags
              VMA_MEMORY_USAGE_CPU_TO_GPU) // memory usage flags
    {
        ubo_buffer.map();
        update();
    }

    // after changes to data in this object, we need explicit update call
    // to push to GPU memory.
    void update()
    {
        for(size_t i = 0; i < ubo_arr.size(); ++i)
        {
            Offset offset = i * aligned_t_size;
            Size size = sizeof(T);
            ubo_buffer.copyToBuffer(&ubo_arr[i], offset, size);
        }
    }

    void update(Offset offset, Size size)
    {
        assert(offset.offset / aligned_t_size == 0);
        static_assert(offset.offset + size.size < N * aligned_t_size);

        std::byte* array_data = (std::byte*)ubo_arr.data() + offset.offset;
        ubo_buffer.copyToBuffer(array_data, offset, size);
    }

    void update(size_t index)
    {
        assert(index < N);
        auto offset = aligned_t_size * index;
        std::byte* array_mem = (std::byte*)&ubo_arr[index];

        ubo_buffer.copyToBuffer(array_mem, Offset{offset}, Size{sizeof(T)});
    }

    const VkDescriptorBufferInfo& getDescriptorWholeBuffer() const
    {
        return ubo_buffer.getDescriptor();
    }

    std::vector<VkDescriptorBufferInfo> getDescriptorBufferInfos() const
    {
        std::vector<VkDescriptorBufferInfo> descriptors;

        auto bufferDescriptor = getDescriptorWholeBuffer();
        bufferDescriptor.range = sizeof(T);

        // so we create N descriptors with range sizeof(T)
        // and with different offsets. One T - one descriptorBufferInfo.
        for (size_t i = 0; i < N; ++i) {
            bufferDescriptor.offset = aligned_t_size * i;
            descriptors.emplace_back(bufferDescriptor);
        }

        return descriptors;
    }

    T* data() { return ubo_arr.data(); }

    T* buf_data() { return (T*)ubo_buffer.mem(); }

    auto begin() const
    {
        return ubo_arr.begin();
    }

    auto end() const
    {
        return ubo_arr.end();
    }

    T& operator[](size_t index)
    {
        assert(index < N);
        return ubo_arr[index];
    }

private:
    std::array<T, N> ubo_arr;
    size_t aligned_t_size;
    VmaVulkanBuffer ubo_buffer;
};

}
