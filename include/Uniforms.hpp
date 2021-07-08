#pragma once
#define GLFW_INCLUDE_VULKAN
#include "VulkanDevice.hpp"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>

// so what will this do?
// A self-contained uniform class? What are my dependencies.
// Certainly Pipeline will need layout of those uniforms, so
// those need to be created before pipeline.
// I will also need a fucking pool
namespace render::memory {

class Uniforms
// so basically, this is a fully fledged uniform management class, no?
// My struct Uniforms should be separate.
{
public:
    Uniforms() { }

private:
    void createDescriptorSets(size_t num_of_swapchain_images);
    void createDescriptorPool(const VulkanDevice&, size_t num_of_swapchain_images);
    VkDescriptorPool descriptor_pool;
    std::vector<VkDescriptorSet> descriptor_sets;
};

} // namespace render::memory
