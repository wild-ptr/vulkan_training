#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <array>

namespace render
{
template<unsigned N>
using AttributeDescriptors = std::array<VkVertexInputAttributeDescription, N>;
} // namespace render

// hpp macros
#define RF_VULKAN_VERTEX_DESCRIPTORS(numOfAttributes) \
    const static VkVertexInputBindingDescription bindingDescriptor; \
    const static AttributeDescriptors<(numOfAttributes)> attributeDescriptors; \
    const static size_t rf_attrib_number = (numOfAttributes);

#define RF_VULKAN_VERTEX_DESCRIPTORS_GETTERS \
    const auto& getBindingDescriptor() const { return bindingDescriptor; }\
    const auto& getAttributeDescriptors() const { return attributeDescriptors; }

// cpp macros
#define RF_VULKAN_VERTEX_DESCRIPTORS_DEFINE_BINDING(Class, Binding) \
    const VkVertexInputBindingDescription Class::bindingDescriptor = (Binding)

#define RF_VULKAN_VERTEX_DESCRIPTORS_DEFINE_ATTRIBUTES(Class, Attributes) \
    const AttributeDescriptors<Class::rf_attrib_number> \
        Class::attributeDescriptors = (Attributes)