#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <array>

namespace render {
template <unsigned N>
using AttributeDescriptors = std::array<VkVertexInputAttributeDescription, N>;
} // namespace render

/* ---------------------------VERTEX MACROS ---------------------- */
// hpp macros
#define RF_VULKAN_VERTEX_DESCRIPTORS_STATIC(numOfAttributes)                   \
    const static VkVertexInputBindingDescription bindingDescriptor;            \
    const static AttributeDescriptors<(numOfAttributes)> attributeDescriptors; \
    const static size_t rf_attrib_number = (numOfAttributes);

#define RF_VULKAN_VERTEX_DESCRIPTORS_GETTERS_STATIC                               \
    static const auto& getBindingDescriptor() { return bindingDescriptor; }       \
    static const auto& getAttributeDescriptors() { return attributeDescriptors; } \
    static size_t getAttributeCount() { return rf_attrib_number; }

// cpp macros
#define RF_VULKAN_VERTEX_DESCRIPTORS_DEFINE_BINDING(Class, Binding) \
    const VkVertexInputBindingDescription Class::bindingDescriptor = (Binding)

#define RF_VULKAN_VERTEX_DESCRIPTORS_DEFINE_ATTRIBUTES(Class, Attributes) \
    const AttributeDescriptors<Class::rf_attrib_number>                   \
        Class::attributeDescriptors = (Attributes)

/* ---------------------------UNIFORM MACROS ---------------------- */
// hpp macros
#define RF_VULKAN_UNIFORM_SET_LAYOUT_STATIC \
    const static VkDescriptorSetLayout uniform_set_layout;

#define RF_VULKAN_UNIFORM_SET_LAYOUT_STATIC_GETTERS \
    static const auto& getDescriptorSetLayout() { return uniform_set_layout; }

// cpp macros
#define RF_VULKAN_UNIFORM_SET_LAYOUT_DEFINE(Class, SetLayout) \
    const VkDescriptorSetLayout Class::uniform_set_layout = SetLayout
