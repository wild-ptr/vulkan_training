#pragma once
#include <glm/glm.hpp>

#include "MemoryStruct.hpp"

namespace render
{

struct Vertex
{
    RF_VULKAN_VERTEX_DESCRIPTORS_GETTERS

    Vertex() = default;
    Vertex(glm::vec2, glm::vec3);

    glm::vec2 pos{};
    glm::vec3 color{};

private:
    RF_VULKAN_VERTEX_DESCRIPTORS(2)
};

} // namespace render
