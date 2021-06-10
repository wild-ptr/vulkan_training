#pragma once
#include "Logger.hpp"
#include <stdexcept>

#define VK_CHECK(function_call, message) \
do { \
    if ((function_call) != VK_SUCCESS) \
    { \
        dbgE << (message) << " error code: " << int(function_call) << NEWL; \
        throw std::runtime_error((message)); \
    } \
}while(0)

