#include "DataTypes/BasicUniformData.hpp"

RF_VULKAN_UNIFORM_SET_LAYOUT_DEFINE(BasicUniformData,
    []{
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
            uboLayoutBinding.binding = 0;
            uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uboLayoutBinding.descriptorCount = 1;
            uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = 1;
            layoutInfo.pBindings = &uboLayoutBinding;

        VkDescriptorSetLayout layout;

        //vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout);

        return layout;
    }());
