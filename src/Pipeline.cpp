#include "Pipeline.hpp"
#include "Logger.hpp"
#include "VulkanMacros.hpp"
#include <algorithm>

namespace render {
Pipeline::~Pipeline()
{
    if (pipelineLayout != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

    if (pipeline != VK_NULL_HANDLE)
        vkDestroyPipeline(device, pipeline, nullptr);
}

Pipeline::Pipeline(Pipeline&& rhs)
    : pipeline(rhs.pipeline)
    , descriptorSetLayouts(std::move(rhs.descriptorSetLayouts))
    , pipelineLayout(rhs.pipelineLayout)
    , device(rhs.device)
{
    rhs.pipeline = VK_NULL_HANDLE;
    rhs.pipelineLayout = VK_NULL_HANDLE;
}

Pipeline& Pipeline::operator=(Pipeline&& rhs)
{
    pipeline = rhs.pipeline;
    pipelineLayout = rhs.pipelineLayout;
    descriptorSetLayouts = std::move(rhs.descriptorSetLayouts);

    rhs.pipeline = VK_NULL_HANDLE;
    rhs.pipelineLayout = VK_NULL_HANDLE;
    return *this;
}

void Pipeline::createPipelineLayout(const Shader& shader)
{
    const auto& descriptorSetLayouts = shader.getReflectedDescriptorSetLayouts();
    const auto& pushConstantRange = shader.getPushConstantRange();

    const auto pipelineLayoutInfo = [&descriptorSetLayouts, &pushConstantRange]() {
        VkPipelineLayoutCreateInfo pli {};

        pli.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pli.setLayoutCount = descriptorSetLayouts.size();
        pli.pSetLayouts = descriptorSetLayouts.data();

        if(pushConstantRange.size > 0)
        {
            pli.pPushConstantRanges = &pushConstantRange;
            pli.pushConstantRangeCount = 1;
        }

        return pli;
    }();

    VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout));
}

} // namespace render
