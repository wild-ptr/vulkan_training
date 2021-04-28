#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


// this class is to be derived from. It is a simple mixin
// giving DerivedType access to bidnding description.

template<typename Derived>
class MemoryStructBase
{
    MemoryStructBase(VkVertexInputBindingDescription desc)
        : bindingDescription(std::move(desc))
    {}

    Derived* getStruct()
    {
        return static_cast<Derived*>(this);
    }

    virtual ~MemoryStructBase() = default;

    VkVertexInputBindingDescription getBindingDescription()
    {
        return bindingDescription;
    }

private:
    VkVertexInputBindingDescription bindingDescription;
};
