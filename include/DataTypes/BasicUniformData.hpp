#pragma once
#define GLFW_INCLUDE_VULKAN
#include "MemoryStruct.hpp"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

struct BasicUniformData {
    RF_VULKAN_UNIFORM_SET_LAYOUT_STATIC_GETTERS

    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::mat3 norm;

private:
    RF_VULKAN_UNIFORM_SET_LAYOUT_STATIC
};
