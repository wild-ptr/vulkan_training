#pragma once
// This should be used for classes that hold Vulkan opaque handles to
// ensure sane default move construction. Default move ctor will copy
// the handles to other object without VK_NULL_HANDL-ing them.
// So, in dtors, the if(handle) checks will still be valid and destroy
// resource that we just moved.

template <typename T>
class UniqueVkHandle
{
public:
    UniqueVkHandle(T data) : data(std::move(data)) {}
    UniqueVkHandle(const UniqueHandle&) = delete;
    UniqueVkHandle(UniqueHandle&& other)
        : data(std::move(other.data))
    {
        other.data = VK_NULL_HANDLE;
    }

    operator T()
    {
        return data;
    }

private:
    T data;
};
