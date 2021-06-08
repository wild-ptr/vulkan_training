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

	VkDevice getDevice() { return vkLogicalDevice; }
	VkPhysicalDevice getPhysicalDevice() { return vkPhysicalDevice; }
	uint32_t getGraphicsQueueIndice() { return queueIndices.graphicsFamily.value(); };
	uint32_t getPresentationQueueIndice() { return queueIndices.presentationFamily.value(); }
	const QueueFamiliesIndices& getQueueIndices() { return queueIndices; }
	VkQueue getGraphicsQueue() { return graphicsQueue; }
	VkQueue getPresentationQueue() { return presentationQueue; }

private:
	VkInstance instance;
	VkSurfaceKHR surface;
    VkPhysicalDevice vkPhysicalDevice;
	QueueFamiliesIndices queueIndices;
    VkDevice vkLogicalDevice;
    VkQueue graphicsQueue;
    VkQueue presentationQueue;
};

} // namespace render
