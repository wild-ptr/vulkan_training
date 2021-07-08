#pragma once
#include "MemoryStruct.hpp"
#include <glm/glm.hpp>

namespace render {

struct Vertex {
    RF_VULKAN_VERTEX_DESCRIPTORS_GETTERS_STATIC

    Vertex() = default;
    Vertex(glm::vec3 pos, glm::vec3 surf_normals, glm::vec3 tangents, glm::vec2 tex_coords);

    glm::vec3 pos {};
    glm::vec3 surf_normals {};
    glm::vec3 tangents {};
    glm::vec2 tex_coords {};

private:
    RF_VULKAN_VERTEX_DESCRIPTORS_STATIC(4)
};

} // namespace render
