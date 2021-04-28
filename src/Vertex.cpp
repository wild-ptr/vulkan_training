#include "Vertex.hpp"
#include <memory>

namespace render
{

RF_VULKAN_VERTEX_DESCRIPTORS_DEFINE_BINDING(Vertex,
    []()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }());

RF_VULKAN_VERTEX_DESCRIPTORS_DEFINE_ATTRIBUTES(Vertex,
    []()
    {
        AttributeDescriptors<2> desc;

        desc[0].binding = 0;
        desc[0].location = 0;
        desc[0].format = VK_FORMAT_R32G32_SFLOAT;
        desc[0].offset = offsetof(Vertex, pos);

        desc[1].binding = 0;
        desc[1].location = 1;
        desc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        desc[1].offset = offsetof(Vertex, color);

        return desc;
    }());

Vertex::Vertex(glm::vec2 pos, glm::vec3 color)
    : pos(std::move(pos))
    , color(std::move(color))
{
}

} // namespace render
