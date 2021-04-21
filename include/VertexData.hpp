#pragma once

namespace render
{

class VertexData
{
    VertexData(VkBuffer buffer);
    ~VertexData();
    VkPipelineVertexInputStateCreateInfo getCi();

private:
    VkPipelineVertexInputStateCreateInfo vertexInputStateCi;
};

} // namespace render
