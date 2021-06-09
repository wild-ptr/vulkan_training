#pragma once
#include "Shader.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <algorithm>
#include "Logger.hpp"

namespace render
{

class Pipeline
{
public:

struct PipelineNoInput{};

template <typename T>
struct vertex_input_tag
{
    using type = T;
};

inline static auto noInputTag{vertex_input_tag<PipelineNoInput>{}};

template<typename InputVertexTag = decltype(noInputTag),
         typename InputVertexFormat = typename InputVertexTag::type>
Pipeline(const std::vector<Shader>& shaders,
                   VkExtent2D swapChainExtent,
                   VkDevice device,
                   VkRenderPass renderPass,
                   InputVertexTag vertexFormat = noInputTag)
    : device(device)
{
    std::vector<VkPipelineShaderStageCreateInfo> shaderStagesCi{};
    std::transform(std::cbegin(shaders), std::cend(shaders), std::back_inserter(shaderStagesCi),
            [](const auto& elem) -> VkPipelineShaderStageCreateInfo
            {
                return elem.getCi();
            });

    for(auto& elem : shaderStagesCi)
    {
        dbgI << "Shader data" << elem.pName << " module: " << elem.module << NEWL;
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};

    if constexpr(std::is_same_v<PipelineNoInput, InputVertexFormat>)
    {
        vertexInputInfo = []
        {
            VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
            vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertexInputInfo.vertexBindingDescriptionCount = 0;
            vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
            vertexInputInfo.vertexAttributeDescriptionCount = 0;
            vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional
            return vertexInputInfo;
        }();
    }
    else
    {
        dbgI << "Creating meaningful vertexInputInfo" << NEWL;
        vertexInputInfo = []
        {
            VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
            vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

            // I only support one vertex binding description now, but this can
            // be extended in the future.
            vertexInputInfo.vertexBindingDescriptionCount = 1;
            vertexInputInfo.pVertexBindingDescriptions =
                std::addressof(InputVertexFormat::getBindingDescriptor());

            vertexInputInfo.vertexAttributeDescriptionCount =
                InputVertexFormat::getAttributeCount();
            vertexInputInfo.pVertexAttributeDescriptions =
                InputVertexFormat::getAttributeDescriptors().data();

            return vertexInputInfo;
        }();
    }

    const auto inputAssembly = []
    {
        VkPipelineInputAssemblyStateCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        ci.primitiveRestartEnable = VK_FALSE;
        return ci;
    }();

    const auto viewport = [&]
    {
        VkViewport vp{};
        vp.x = 0.0f;
        vp.y = 0.0f;
        vp.width = (float) swapChainExtent.width;
        vp.height = (float) swapChainExtent.height;
        vp.minDepth = 0.0f;
        vp.maxDepth = 1.0f;
        return vp;
    }();

    const auto scissor = [&]
    {
        VkRect2D scissor;
        scissor.offset = {0,0};
        scissor.extent = swapChainExtent;
        return scissor;
    }();

    const auto viewportState = [&viewport, &scissor]
    {
        VkPipelineViewportStateCreateInfo vs{};
        vs.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        vs.viewportCount = 1;
        vs.pViewports = &viewport;
        vs.scissorCount = 1;
        vs.pScissors = &scissor;

        return vs;
    }();

    const auto rasterizer = []
    {
        VkPipelineRasterizationStateCreateInfo rs{};
        rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rs.depthClampEnable = VK_FALSE;
        rs.rasterizerDiscardEnable = VK_FALSE; // otherwise no rasterization will occur.
        rs.polygonMode = VK_POLYGON_MODE_FILL;
        rs.lineWidth = 1.0f;

        rs.cullMode = VK_CULL_MODE_BACK_BIT;
        rs.frontFace = VK_FRONT_FACE_CLOCKWISE;

        rs.depthBiasEnable = VK_FALSE;

        return rs;
    }();

    const auto multisampling = []
    {
        VkPipelineMultisampleStateCreateInfo ms{};
        ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        ms.sampleShadingEnable = VK_FALSE;
        ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        return ms;
    }();

    const auto colorBlendAttachment = []
    {
        VkPipelineColorBlendAttachmentState cba{};
        cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                             VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        cba.blendEnable = VK_TRUE;
        cba.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        cba.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        cba.colorBlendOp = VK_BLEND_OP_ADD;
        cba.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        cba.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        cba.alphaBlendOp = VK_BLEND_OP_ADD;
        return cba;
    }();

    const auto colorBlend = [&colorBlendAttachment]
    {
        VkPipelineColorBlendStateCreateInfo cb{};
        cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        cb.logicOpEnable = VK_FALSE;
        cb.logicOp = VK_LOGIC_OP_COPY;
        cb.attachmentCount = 1;
        cb.pAttachments = &colorBlendAttachment;

        return cb;
    }();

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_LINE_WIDTH
    };

    const auto dynamicState = [&dynamicStates]
    {
        VkPipelineDynamicStateCreateInfo ds{};
        ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        ds.dynamicStateCount = 2;
        ds.pDynamicStates = dynamicStates;
        return ds;
    }();

    // This is used to pass uniforms. Empty for now.
    const auto pipelineLayoutInfo = []
    {
        VkPipelineLayoutCreateInfo pli{};
        pli.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        return pli;
    }();

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout)
            != VK_SUCCESS)
        throw std::runtime_error("Failed to create pipeline layout.");

    const auto pipelineInfo = [&]
    {
        VkGraphicsPipelineCreateInfo pci{};
        pci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pci.stageCount = shaderStagesCi.size();
        pci.pStages = shaderStagesCi.data();

        pci.pVertexInputState = &vertexInputInfo;
        pci.pInputAssemblyState = &inputAssembly;
        pci.pViewportState = &viewportState;
        pci.pRasterizationState = &rasterizer;
        pci.pMultisampleState = &multisampling;
        pci.pDepthStencilState = nullptr;
        pci.pColorBlendState = &colorBlend;
        pci.pDynamicState = nullptr; // why

        pci.layout = pipelineLayout;
        pci.renderPass = renderPass;
        pci.subpass = 0; // so i can only choose one?

        return pci;
    }();

    if(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                &pipeline) != VK_SUCCESS)
        throw std::runtime_error("Failed to create graphics pipeline");
}

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
