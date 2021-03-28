#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <optional>


namespace vk_main
{

struct QueueFamiliesIndices
{   // will get more populated in the future.
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentationFamily;

    bool hasAllMembers()
    {
        return graphicsFamily.has_value() and
               presentationFamily.has_value();
    }
};


class VulkanApplication
{
public:
	void run();

private:
	void initWindow();
	void initVulkan();
	void mainLoop();
	void cleanup(); // no RAII for now.
    void createSurface();

// members
	const size_t WIDTH = 1024;
	const size_t HEIGHT = 768;
	GLFWwindow* window;
	VkInstance vkInstance;
	VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice vkPhysicalDevice;
    VkDevice vkLogicalDevice;
    VkQueue graphicsQueue;
    VkQueue presentationQueue;
    VkSurfaceKHR surface;
    QueueFamiliesIndices indices;
};

} // namespace vk_main
