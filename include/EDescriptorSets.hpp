#pragma once

// Not enum class to allow implicit casting to ints.
// Maps set number to rebind frequency.
namespace render::EDescriptorSets {
enum EDescriptorSets : uint8_t
{
    BindFrequency_Frame = 0,
    BindFrequency_Object = 1,
};
}
