#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <iostream>

#include "VulkanApplication.hpp"

int main()
{
    render::VulkanApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
