#pragma once
#include <memory>
#include "VulkanDevice.hpp"
#include "TextureManager.hpp"
#include "UniformData.hpp"
#include "Constants.hpp"
#include "Pipeline.hpp"

namespace render::memory
{

// more things will be added in the future.
class PerFrameUniformSystem
{
public:
    // can this really only be compatible with one pipeline layout? damn.
    PerFrameUniformSystem(std::shared_ptr<VulkanDevice> device,
                          std::shared_ptr<TextureManager> texmgr,
                          std::shared_ptr<Pipeline> pipeline);

    void refreshData(uint32_t setIdx);
    void bind(VkCommandBuffer buf, uint32_t setIdx);

private:
    void createDescriptorPool();
    void generateDescriptorSets();
    void fillDescriptorSets();

    std::shared_ptr<VulkanDevice> device;
    std::shared_ptr<memory::TextureManager> texture_mgr;
    std::shared_ptr<Pipeline> pipeline;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
};

} // namespace render
