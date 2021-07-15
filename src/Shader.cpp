#include <cassert>
#include <vector>

#include "Logger.hpp"
#include "Shader.hpp"
#include "VulkanMacros.hpp"
#include "spirv_reflect/printers.h"
#include "spirv_reflect/spirv_reflect.h"
#include "utils.hpp"

namespace render {

namespace {

    std::vector<VkDescriptorSetLayout> createDescriptorSetLayouts(
        VkDevice device,
        const std::vector<DescriptorSetLayoutData>& setLayoutData)
    {
        std::vector<VkDescriptorSetLayout> ret(setLayoutData.size());

        size_t idx = 0;
        for (const auto& set : setLayoutData) {
            VkDescriptorSetLayoutCreateInfo ci {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .bindingCount = set.bindings.size(),
                .pBindings = set.bindings.data()
            };

            VK_CHECK(vkCreateDescriptorSetLayout(device, &ci, nullptr, &ret[idx]));
            ++idx;
        }

        return ret;
    }

    std::vector<DescriptorSetLayoutData> reflectDescriptorSets(
        const std::vector<char>& bytecode)
    {
        SpvReflectShaderModule module = {};
        SpvReflectResult result = spvReflectCreateShaderModule(bytecode.size(), bytecode.data(), &module);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        uint32_t count = 0;
        result = spvReflectEnumerateDescriptorSets(&module, &count, NULL);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        // there is no point to parse if there are no descriptors.
        if (count == 0)
            return {};

        std::vector<SpvReflectDescriptorSet*> sets(count);
        result = spvReflectEnumerateDescriptorSets(&module, &count, sets.data());
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        std::vector<DescriptorSetLayoutData> set_layouts(sets.size(), DescriptorSetLayoutData {});

        for (size_t i_set = 0; i_set < sets.size(); ++i_set) {
            const SpvReflectDescriptorSet& refl_set = *(sets[i_set]);
            DescriptorSetLayoutData& layout = set_layouts[i_set];
            layout.bindings.resize(refl_set.binding_count);

            for (uint32_t i_binding = 0; i_binding < refl_set.binding_count; ++i_binding) {
                const SpvReflectDescriptorBinding& refl_binding = *(refl_set.bindings[i_binding]);
                VkDescriptorSetLayoutBinding& layout_binding = layout.bindings[i_binding];
                layout_binding.binding = refl_binding.binding;
                layout_binding.descriptorType = static_cast<VkDescriptorType>(refl_binding.descriptor_type);
                layout_binding.descriptorCount = 1;

                for (uint32_t i_dim = 0; i_dim < refl_binding.array.dims_count; ++i_dim) {
                    layout_binding.descriptorCount *= refl_binding.array.dims[i_dim];
                }

                layout_binding.stageFlags = static_cast<VkShaderStageFlagBits>(module.shader_stage);
            }

            layout.set_number = refl_set.set;
        }

        // Log the descriptor set contents to stdout
        // @TODO use my logger.
        const char* t = "  ";
        const char* tt = "    ";

        spirv_reflect::PrintModuleInfo(std::cout, module, "");
        std::cout << "Descriptor sets:"
                  << "\n";
        for (size_t index = 0; index < sets.size(); ++index) {
            auto p_set = sets[index];

            std::cout << t << index << ":"
                      << "\n";
            spirv_reflect::PrintDescriptorSet(std::cout, *p_set, tt);
            std::cout << "\n\n";
        }

        spvReflectDestroyShaderModule(&module);
        return set_layouts;
    }

    VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice device)
    {
        const auto createInfo = [&code]() {
            VkShaderModuleCreateInfo ci {};
            ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            ci.codeSize = code.size();
            ci.pCode = reinterpret_cast<const uint32_t*>(code.data());
            return ci;
        }();

        VkShaderModule shaderModule;

        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
            throw std::runtime_error("Cannot create shadermodule from code.");

        dbgI << "CreateShaderModule handle result: " << shaderModule << NEWL;
        return shaderModule;
    }

    VkShaderStageFlagBits getVulkanStageType(render::EShaderType type)
    {
        switch (type) {
        case (render::EShaderType::VERTEX_SHADER):
            return VK_SHADER_STAGE_VERTEX_BIT;
        case (render::EShaderType::FRAGMENT_SHADER):
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        default:
            throw std::runtime_error("Unsupported shader type... yet!");
        }
    }

    VkPipelineShaderStageCreateInfo makeShaderCreateInfo(render::EShaderType type,
        const VkShaderModule& module)
    {
        return [&] {
            VkPipelineShaderStageCreateInfo ci {};
            ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            ci.stage = getVulkanStageType(type);
            ci.module = module;
            ci.pName = "main"; // shader entrypoint.

            return ci;
        }();
    }

} // anonymous namespace

Shader::Shader(VkDevice device, const std::string& path, EShaderType type)
    : device(device)
    , shaderType(getVulkanStageType(type))
{
    auto vShaderCode = utils::readFileBinary(path);

    shaderModule = std::shared_ptr<VkShaderModule> { new VkShaderModule(createShaderModule(vShaderCode, device)),
        [device](VkShaderModule* p) {
            vkDestroyShaderModule(device, *p, nullptr);
            delete p;
        } };

    createInfo = makeShaderCreateInfo(type, *shaderModule);
    setLayoutData = reflectDescriptorSets(vShaderCode);
    descriptorSetLayouts = createDescriptorSetLayouts(device, setLayoutData);
}

Shader::Shader(Shader&& rhs)
    : createInfo(rhs.createInfo)
    , shaderModule(std::move(rhs.shaderModule))
    , device(rhs.device)
    , shaderType(rhs.shaderType)
    , setLayoutData(std::move(rhs.setLayoutData))
    , descriptorSetLayouts(std::move(rhs.descriptorSetLayouts))
{
    //rhs.shaderModule = VK_NULL_HANDLE;
    rhs.shaderModule.reset();
    rhs.device = VK_NULL_HANDLE;
    rhs.createInfo = decltype(rhs.createInfo) {};
}

Shader::~Shader()
{
    if (shaderModule != VK_NULL_HANDLE) {
        //vkDestroyShaderModule(device, shaderModule, nullptr);
    }
}

VkPipelineShaderStageCreateInfo Shader::getCi() const
{
    return createInfo;
}

VkShaderModule Shader::getShaderModule() const
{
    return *shaderModule;
}

VkDevice Shader::getDevice() const
{
    return device;
}

const std::vector<DescriptorSetLayoutData>& Shader::getSetLayoutData() const
{
    return setLayoutData;
}

const std::vector<VkDescriptorSetLayout>& Shader::getReflectedDescriptorSetLayouts() const
{
    return descriptorSetLayouts;
}

VkShaderStageFlagBits Shader::getShaderType() const
{
    return shaderType;
}

} // namespace render
