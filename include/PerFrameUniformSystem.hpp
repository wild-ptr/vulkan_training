#pragma once
#include <memory>
#include "VulkanDevice.hpp"
#include "TextureManager.hpp"
#include "UniformData.hpp"
#include "Constants.hpp"
#include "Pipeline.hpp"
#include "CameraSystem.hpp"

namespace render::memory
{

// other junk such as light list will also get there.
struct PerFrameUbo
{
    alignas(16) CameraSystem::UboData camera;
};

// more things will be added in the future.
class PerFrameUniformSystem
{
public:
    // can this really only be compatible with one pipeline layout? damn.
    PerFrameUniformSystem(std::shared_ptr<VulkanDevice> device,
                          std::shared_ptr<TextureManager> texmgr,
                          std::shared_ptr<CameraSystem> camera,
                          std::shared_ptr<Pipeline> pipeline);

    void refreshData(uint32_t setIdx);
    void bind(VkCommandBuffer buf, uint32_t setIdx);

private:
    void createDescriptorPool();
    void generateDescriptorSets();
    void fillDescriptorSets();
    void fillUboDescriptor();

    std::shared_ptr<VulkanDevice> device;
    std::shared_ptr<TextureManager> texture_mgr;
    std::shared_ptr<CameraSystem> camera;
    std::shared_ptr<Pipeline> pipeline;
    std::unique_ptr<UniformData<PerFrameUbo, consts::maxFramesInFlight>> ubo;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
};

} // namespace render
