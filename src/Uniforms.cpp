#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "Uniforms.hpp"
#include "VulkanDevice.hpp"

namespace render::memory
{

void Uniforms::createDescriptorPool(const VulkanDevice& device, size_t num_of_swapchain_images)
{
    // First we describe which descriptor types our sets will contain (uniform descriptors),
    // and how many of them will be allocated.
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   	poolSize.descriptorCount = static_cast<uint32_t>(num_of_swapchain_images);

    VkDescriptorPoolCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        ci.poolSizeCount = 1;
        ci.pPoolSizes = &poolSize;
        ci.maxSets = static_cast<uint32_t>(num_of_swapchain_images);

    vkCreateDescriptorPool(device.getDevice(), &ci, nullptr, &descriptor_pool);
}

void Uniforms::createDescriptorSets(size_t num_of_swapchain_images)
{
 //   // unfortunately layouts must be copied despite being identical. One layout per set,
 //   // and we will have as many sets as swapchain images, as is tradition.
 //   std::vector<VkDescriptorSetLayout> layouts(num_of_swapchain_images, descriptorSetLayout);
 //
 //   VkDescriptorSetAllocateInfo allocInfo{};
 //       allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
 //       allocInfo.descriptorPool = descriptorPool;
 //       allocInfo.descriptorSetCount = static_cast<uint32_t>(num_of_swapchain_images);
 //       allocInfo.pSetLayouts = layouts.data();
 //
 //   descriptorSets.resize(num_of_swapchain_images);
 //   vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data());
 //
 //   // We need to bind descriptor set to a buffer! This will make the descriptor set complete.
 //   for(size_t i = 0; i < num_of_swapchain_images; ++i)
 //   {
 //       VkDescriptorBufferInfo bi{};
 //           bi.buffer = uniformBuffers[i];
 //           bi.offset = 0;
 //           bi.range = VK_WHOLE_SIZE; // or sizeof(UniformBufferObject);
 //
 //       VkWriteDescriptorSet descriptorWrite{};
 //           descriptorWrite.dstSet = descriptorSets[i];
 //           descriptorWrite.dstBinding = 0;
 //           descriptorWrite.dstArrayElement = 0;
 //
 //           descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
 //           descriptorWrite.descriptorCount = 1; // its possible to update multiple descriptors
 //           // at once in an array, starting at index dstArrayElement;
 //
 //           descriptorWrite.pBufferInfo = &bi;
 //
 //       // now this can be a data race, we need to be sure GPU is not doing anything
 //       // with this buffer before we call UpdateDescriptorSets. It is not a vkCmd.
 //       vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
 //   }
}

} // namespace render::memory
