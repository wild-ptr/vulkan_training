#pragma once
#include "Logger.hpp"
#include <stdexcept>

#define VK_CHECK(function_call) \
do { \
    if ((function_call) != VK_SUCCESS) \
    { \
        dbgE << "Vulkan call: " << #function_call << " error code: " << int(function_call) << NEWL; \
        throw std::runtime_error(#function_call); \
    } \
}while(0)

