#pragma once
#include "spirv_reflect/spirv_reflect.h"

namespace spirv_reflect
{

void PrintDescriptorBinding(
        std::ostream& os,
        const SpvReflectDescriptorBinding& obj,
        bool write_set,
        const char* indent)
{
    const char* t = indent;
    os << t << "binding : " << obj.binding << "\n";
    if (write_set)
    {
        os << t << "set     : " << obj.set << "\n";
    }

    // array
    if (obj.array.dims_count > 0) {
        os << t << "array   : ";
        for (uint32_t dim_index = 0; dim_index < obj.array.dims_count; ++dim_index)
        {
            os << "[" << obj.array.dims[dim_index] << "]";
        }
        os << "\n";
    }

    // counter
    if (obj.uav_counter_binding != nullptr)
    {
      os << t << "counter : ";
      os << "(";
      os << "set=" << obj.uav_counter_binding->set << ", ";
      os << "binding=" << obj.uav_counter_binding->binding << ", ";
      os << "name=" << obj.uav_counter_binding->name;
      os << ");";
      os << "\n";
    }

    os << t << "name    : " << obj.name;

    if ((obj.type_description->type_name != nullptr) && (strlen(obj.type_description->type_name) > 0))
    {
        os << " " << "(" << obj.type_description->type_name << ")";
    }
}

void PrintModuleInfo(std::ostream& os, const SpvReflectShaderModule& obj, const char* /*indent*/)
{
    os << "entry point     : " << obj.entry_point_name << "\n";
    os << "source lang     : " << spvReflectSourceLanguage(obj.source_language) << "\n";
    os << "source lang ver : " << obj.source_language_version << "\n";
    if (obj.source_language == SpvSourceLanguageHLSL)
    {
        os << "stage           : ";
        switch (obj.shader_stage)
        {
            default: break;
            case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT                   : os << "VS"; break;
            case SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT     : os << "HS"; break;
            case SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT  : os << "DS"; break;
            case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT                 : os << "GS"; break;
            case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT                 : os << "PS"; break;
            case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT                  : os << "CS"; break;
        }
    }
}

void PrintDescriptorSet(std::ostream& os, const SpvReflectDescriptorSet& obj, const char* indent)
{
    const char* t     = indent;
    std::string tt    = std::string(indent) + "  ";
    std::string ttttt = std::string(indent) + "    ";

    os << t << "set           : " << obj.set << "\n";
    os << t << "binding count : " << obj.binding_count;
    os << "\n";

    for (uint32_t i = 0; i < obj.binding_count; ++i)
    {
        const SpvReflectDescriptorBinding& binding = *obj.bindings[i];
        os << tt << i << ":" << "\n";
        PrintDescriptorBinding(os, binding, false, ttttt.c_str());

        if (i < (obj.binding_count - 1))
        {
            os << "\n";
        }
    }
}

} // namespace spirv_reflect
