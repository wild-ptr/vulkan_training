#pragma once
#include <GLFW/glfw3.h>
#include "VulkanImage.hpp"

// This contins information about which framebuffer images, and thus
// all other duplicate resources, are taken.
// Supports sleeping on fences and stuff. MT-safe.

namespace render::sync
{

// tag class to vary construction policy.
class NullHandleContainerTag{};
inline static NullHandleContainerTag createNullHandleContainer;

class FenceContainer
{
    FenceContainer(VulkanDevice& device, size_t size)
        : vkDevice(device)
    {
        fences.resize(size);

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for(size_t i = 0; i < size(); ++i)
        {
            // cannot create fence in place for atomics.
            VkFence tempFence;
            vkCreateFence(vkDevice.getDevice(), &fenceInfo, nullptr,
                &tempFence);

            fences[i] = tempFence;
        }
    }

    FenceContainer(VulkanDevice& device, size_t size, NullHandleContainerTag)
    {
        // all VK_NULL_HANDLE's
        fences.resize(size);
    }

    // no-op on VK_NULL_HANDLE
    void waitForFence(size_t index) const
    {
        if(fences[index].load() == VK_NULL_HANDLE)
        {
            return;
        }

        vkWaitForFences(vkDevice.getDevice(), 1, fences[index].load(),
                VK_TRUE, UINT64_MAX);
    }

    // no-op on VK_NULL_HANDLE
    void resetFence(size_t index)
    {
        if(fences[index] != VK_NULL_HANDLE)
        {
            vkResetFences(vkDevice.getDevice(), 1, fences[index].load());
        }
    }

    // todo, as usual
    ~FenceContainer() = default;

    std::atomic<VkFence>& operator[](size_t idx)
    {
        return fences[idx];
    }

    const std::atomic<VkFence>& operator[](size_t idx) const
    {
        return fences[idx];
    }

private:
    std::vector<std::atomic<VkFence>> fences;
    VulkanDevice& vkDevice;
};

} // namespace render::sync
