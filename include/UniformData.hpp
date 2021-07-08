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

template <class T, unsigned N = 1>
class UniformData {
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

    void update(size_t index)
    {
        static_assert(index < ubo_arr.size());
        auto offset = sizeof(T) * index;
        std::byte* array_data = (std::byte*)ubo_arr.data() + offset;
        ubo_buffer.copyToBuffer(array_data, offset, sizeof(T));
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
            bufferDescriptor.offset = sizeof(T) * i;
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

    //std::vector<VkDescriptorSet> getDescriptorSets()
    //{
    //    // this denotes start of the buffer. We will need to generate N
    //    // descriptors.
    //    std::vector<VkDescriptorBufferInfo> descriptors;
    //
    //    auto bufferDescriptor = getDescriptorWholeBuffer();
    //    bufferDescriptor.range = sizeof(T);
    //
    //    // so we create N descriptors with range sizeof(T)
    //    // and with different offsets. One T - one descriptorBufferInfo.
    //    for(size_t i = 0; i < N; ++i)
    //    {
    //        bufferDescriptor.offset = sizeof(T) * i;
    //        descriptors.emplace_back(bufferDescriptor);
    //    }
    //}

private:
    VmaVulkanBuffer ubo_buffer;
    std::array<T, N> ubo_arr;
};

}
