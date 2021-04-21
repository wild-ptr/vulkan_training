#pragma once
#include "Shader.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

namespace render
{
class Pipeline
{
public:
    Pipeline(const std::vector<Shader>& shaders, VkExtent2D, VkDevice, VkRenderPass);
    Pipeline(){};
    Pipeline(Pipeline&&);
    ~Pipeline();
    VkPipeline getHandle() const { return pipeline; }
    VkPipelineLayout getLayoutHandle() const { return pipelineLayout; }

    Pipeline& operator=(Pipeline&&);
private:
    VkPipeline pipeline{VK_NULL_HANDLE};
    VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};
    VkDevice device;
};
} // namespace render
