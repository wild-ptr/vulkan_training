#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace render {
class VulkanInstance {
public:
    VulkanInstance(bool debugFlag);
    VulkanInstance();
    ~VulkanInstance();

    VkInstance getInstance() { return vkInstance; }

private:
    bool validationLayersEnabled;
    VkInstance vkInstance;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkDebugReportCallbackEXT debugCallback = VK_NULL_HANDLE;
};

} // namespace render
