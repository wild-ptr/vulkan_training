#pragma once
#define GLFW_INCLUDE_VULKAN
#include "vk_mem_alloc.h"
#include <GLFW/glfw3.h>
#include <optional>
#include <vector>
#include <functional>

namespace render {
struct QueueFamiliesIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentationFamily;

    bool hasAllMembers()
    {
        return graphicsFamily.has_value() and presentationFamily.has_value();
    }
};

class VulkanDevice {
public:
    VulkanDevice() {}; // dummy ctor to allow deferred filling in. No checking, care.
    VulkanDevice(VkInstance instance, VkSurfaceKHR);

    ~VulkanDevice();

    VkDevice getDevice() const { return vkLogicalDevice; }
    VkPhysicalDevice getPhysicalDevice() const { return vkPhysicalDevice; }
    uint32_t getGraphicsQueueIndice() const { return queueIndices.graphicsFamily.value(); };
    uint32_t getPresentationQueueIndice() const { return queueIndices.presentationFamily.value(); }
    const QueueFamiliesIndices& getQueueIndices() const { return queueIndices; }
    VkQueue getGraphicsQueue() const { return graphicsQueue; }
    VkQueue getPresentationQueue() const { return presentationQueue; }
    VmaAllocator getVmaAllocator() const { return allocator; }
    void immediateSubmitBlocking(std::function<void(VkCommandBuffer)> func);

private:
    VkPhysicalDevice vkPhysicalDevice;
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    VkPhysicalDeviceMemoryProperties deviceMemProperties;
    QueueFamiliesIndices queueIndices;
    VkDevice vkLogicalDevice;
    VkQueue graphicsQueue;
    VkQueue presentationQueue;
    VmaAllocator allocator;

    struct
    {
        VkFence uploadFence;
        VkCommandPool uploadCommandPool;
    } uploadContext;
};

namespace deviceUtils {
    uint32_t getMemoryTypeIndex(
        VkPhysicalDevice physDevice,
        uint32_t mem_type_bits,
        VkMemoryPropertyFlags properties,
        VkBool32* success = nullptr);

    VkMemoryPropertyFlags getMemoryProperties(
        VkPhysicalDevice physDevice,
        uint32_t memIndex);
}

} // namespace render
