#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Constants.hpp"
#include "Mesh.hpp"
#include "Pipeline.hpp"
#include "UniformData.hpp"
#include "VulkanDevice.hpp"

namespace render {

struct RenderableUbo {
    glm::mat4 model;
};

/* This class will represent a renderable entity,
 * which will have its own maxFramesInFlight sets of uniforms,
 * and will contain meshes that are to be drawn on screen. */
class Renderable {
public:
    Renderable(const VulkanDevice& device, std::vector<Mesh> meshes);
    void updateUniforms(const RenderableUbo&, size_t bufferIdx);
    void cmdBindSetsDrawRenderable();

private:
    void createDescriptorPool();
    void generateUboDescriptorSets();

    const VulkanDevice& device;
    std::vector<Mesh> meshes;
    std::shared_ptr<Pipeline> pipeline;
    std::unique_ptr<memory::UniformData<RenderableUbo, consts::maxFramesInFlight>> uniforms;
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
};

} // namespace render
