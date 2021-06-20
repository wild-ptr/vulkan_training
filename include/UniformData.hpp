#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VmaVulkanBuffer.hpp"
#include <array>

namespace render::memory
{

// Wrapper class for uniform data. Treat as standard singular uniform (or array), and .update()
// to make changes gpu-visible. Proper synchronization is still required.
template<class T, unsigned N = 1>
class UniformData
{
public:
    UniformData(VmaAllocator allocator)
        : ubo_buffer(allocator, nullptr, sizeof(T) * N,
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, // buffer usage flags
                     VMA_MEMORY_USAGE_CPU_TO_GPU) // memory usage flags
    {
        // shall be permanently mapped, as uniforms change often.
        ubo_buffer.map();
    }

    UniformData(VmaAllocator allocator, std::array<T, N> data)
        : ubo_arr(std::move(data))
        , ubo_buffer(allocator, nullptr, sizeof(T) * N,
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, // buffer usage flags
                     VMA_MEMORY_USAGE_CPU_TO_GPU) // memory usage flags
    {
        ubo_buffer.map();
    }

    // after changes to data in this object, we need explicit update call
    // to push to GPU memory.
    void update()
    {
        ubo_buffer.copyToBuffer(ubo_arr.data(), N * sizeof(T));
    }

    void update(Offset offset, Size size)
    {
        static_assert(offset.offset + size.size < N);
        std::byte* array_data = (std::byte*)ubo_arr.data() + offset.offset;
        ubo_buffer.copyToBuffer(array_data, offset, size);
    }

    const VkDescriptorBufferInfo& getDescriptor() const { return ubo_buffer.getDescriptor(); }

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
    VmaVulkanBuffer ubo_buffer;
    std::array<T,N> ubo_arr;
};

}
