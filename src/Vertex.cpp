#include "Vertex.hpp"
#include <memory>
#include <cstddef>

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
        AttributeDescriptors<4> desc;

        desc[0].binding = 0;
        desc[0].location = 0;
        desc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        desc[0].offset = offsetof(Vertex, pos);

        desc[1].binding = 0;
        desc[1].location = 1;
        desc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        desc[1].offset = offsetof(Vertex, surf_normals);

        desc[2].binding = 0;
        desc[2].location = 2;
        desc[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        desc[2].offset = offsetof(Vertex, tangents);

        desc[3].binding = 0;
        desc[3].location = 3;
        desc[3].format = VK_FORMAT_R32G32_SFLOAT;
        desc[3].offset = offsetof(Vertex, tex_coords);

        return desc;
    }());

Vertex::Vertex(glm::vec3 pos, glm::vec3 surf_normals, glm::vec3 tangents, glm::vec2 tex_coords)
    : pos(std::move(pos))
    , surf_normals(std::move(surf_normals))
    , tangents(std::move(tangents))
    , tex_coords(std::move(tex_coords))
{
}

} // namespace render
