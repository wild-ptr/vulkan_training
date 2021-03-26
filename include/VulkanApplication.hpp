#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


namespace vk_main
{

class VulkanApplication
{
public:
	void run();

private:
	void initWindow();
	void initVulkan();
	void mainLoop();
	void cleanup(); // no RAII for now.

// members
	GLFWwindow* window;
	const size_t WIDTH = 1024;
	const size_t HEIGHT = 768;
	VkInstance vkInstance;
	VkDebugUtilsMessengerEXT debugMessenger;
};

} // namespace vk_main
