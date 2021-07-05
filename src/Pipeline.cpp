#include <algorithm>
#include "Logger.hpp"
#include "Pipeline.hpp"
#include "VulkanMacros.hpp"

namespace render
{
Pipeline::~Pipeline()
{
    if(pipelineLayout != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

    if(pipeline != VK_NULL_HANDLE)
        vkDestroyPipeline(device, pipeline, nullptr);
}

Pipeline::Pipeline(Pipeline&& rhs)
    : pipeline(rhs.pipeline)
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

    rhs.pipeline = VK_NULL_HANDLE;
    rhs.pipelineLayout = VK_NULL_HANDLE;
    return *this;
}

void Pipeline::createPipelineLayout(const Shader& shader)
{
    const auto& descriptorSetLayouts = shader.getReflectedDescriptorSetLayouts();

    const auto pipelineLayoutInfo = [&descriptorSetLayouts]()
    {
        VkPipelineLayoutCreateInfo pli{};
        pli.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pli.setLayoutCount = descriptorSetLayouts.size();
        pli.pSetLayouts = descriptorSetLayouts.data();
        return pli;
    }();

    VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout));
}

} // namespace render
