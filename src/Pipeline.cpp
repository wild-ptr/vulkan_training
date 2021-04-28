#include "Logger.hpp"
#include "Pipeline.hpp"
#include <algorithm>
#include <stdexcept>

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

} // namespace render
