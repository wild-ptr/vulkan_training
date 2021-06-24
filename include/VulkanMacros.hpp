#pragma once
#include "Logger.hpp"
#include <stdexcept>
#include <chrono>
#include <thread>


// the sleep is defined to give a chance for debug messenger callback to print out info
// before throwing and in effect destroying VkInstance and aborting the program.
#define VK_CHECK(function_call) \
do { \
    if ((function_call) != VK_SUCCESS) \
    { \
        dbgE << "Vulkan call: " << #function_call << " error code: " << int(function_call) << NEWL; \
        using namespace std::literals::chrono_literals; \
        std::this_thread::sleep_for(3s); \
        throw std::runtime_error(#function_call); \
    } \
}while(0)

