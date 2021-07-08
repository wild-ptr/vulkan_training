#pragma once
#include "Logger.hpp"
#include <stdexcept>

#define VK_CHECK(function_call)                                          \
    do {                                                                 \
        auto vk_macro_ret = function_call;                               \
        if (vk_macro_ret != VK_SUCCESS) {                                \
            dbgE << "Vulkan call: " << #function_call << " error code: " \
                 << int(vk_macro_ret) << NEWL;                           \
            throw std::runtime_error(#function_call);                    \
        }                                                                \
    } while (0)
