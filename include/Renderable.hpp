#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <memory>

#include "Constants.hpp"
#include "Mesh.hpp"
#include "Pipeline.hpp"
#include "UniformData.hpp"
#include "VulkanDevice.hpp"

namespace render {

// of course we will make renderable templated in the future to allow plugging in specific UBO's
// for different pipelines.
struct RenderableUbo {
    glm::mat4 model;
    float times;
};

/* This class will represent a renderable entity,
 * which will have its own maxFramesInFlight sets of uniforms,
 * and will contain meshes that are to be drawn on screen. */
class Renderable {
public:
    Renderable(
            std::shared_ptr<VulkanDevice> device,
            std::shared_ptr<Pipeline> pipeline,
            std::vector<Mesh> meshes);
    void updateUniforms(RenderableUbo, size_t bufferIdx);
    void cmdBindSetsDrawMeshes(VkCommandBuffer, uint32_t frameIndex);

private:
    void createDescriptorPool();
    void generateUboDescriptorSets();

    std::shared_ptr<VulkanDevice> device;
    std::vector<Mesh> meshes;
    std::shared_ptr<Pipeline> pipeline;
    std::unique_ptr<memory::UniformData<RenderableUbo, consts::maxFramesInFlight>> uniforms;
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
};

} // namespace render
