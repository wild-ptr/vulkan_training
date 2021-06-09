#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <optional>
#include <vector>

namespace render
{
struct QueueFamiliesIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentationFamily;

    bool hasAllMembers()
    {
        return graphicsFamily.has_value() and
               presentationFamily.has_value();
    }
};

class VulkanDevice
{
public:
	VulkanDevice(){}; // dummy ctor to allow deferred filling in. No checking, care.
	VulkanDevice(VkInstance instance, VkSurfaceKHR);
	~VulkanDevice();

	VkDevice getDevice() const { return vkLogicalDevice; }
	VkPhysicalDevice getPhysicalDevice() const { return vkPhysicalDevice; }
	uint32_t getGraphicsQueueIndice() const { return queueIndices.graphicsFamily.value(); };
	uint32_t getPresentationQueueIndice() const { return queueIndices.presentationFamily.value(); }
	const QueueFamiliesIndices& getQueueIndices() const { return queueIndices; }
	VkQueue getGraphicsQueue() const { return graphicsQueue; }
	VkQueue getPresentationQueue() const { return presentationQueue; }

private:
    VkPhysicalDevice vkPhysicalDevice;
	QueueFamiliesIndices queueIndices;
    VkDevice vkLogicalDevice;
    VkQueue graphicsQueue;
    VkQueue presentationQueue;
};

} // namespace render
