#pragma once
#include <glm/glm.hpp>

#include "MemoryStruct.hpp"

namespace render
{

struct Vertex : MemoryStructBase<Vertex>
{
    Vertex();
    Vertex(glm::vec2, glm::vec3);
    glm::vec2 pos{};
    glm::vec3 color{};
};

} // namespace render
