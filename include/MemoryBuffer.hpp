#pragma once
#include "Vertex.hpp"

namespace render::mem
{

// This class is responsible for allocating objects. It will create
// Raii-Style memory handles. Seems fun for first idea.

class MemoryHandle
{

};

class MemoryBuffer
{
    MemoryBuffer();
    MemoryHandle allocateMemory(const std::vector<Vertex>&);
};

} // namespace render::mem
