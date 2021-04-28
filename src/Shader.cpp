#include "Logger.hpp"
#include "Shader.hpp"
#include "utils.hpp"
#include <vector>

namespace {
VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice device)
{
    const auto createInfo = [&code]()
    {
        VkShaderModuleCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        ci.codeSize = code.size();
        ci.pCode = reinterpret_cast<const uint32_t*>(code.data());
        return ci;
    }();

    VkShaderModule shaderModule;

    if(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        throw std::runtime_error("Cannot create shadermodule from code.");

    dbgI << "CreateShaderModule handle result: " << shaderModule << NEWL;
    return shaderModule;
}

VkShaderStageFlagBits getVulkanStageType(render::EShaderType type)
{
    switch(type)
    {
        case(render::EShaderType::VERTEX_SHADER):
            return VK_SHADER_STAGE_VERTEX_BIT;
        case(render::EShaderType::FRAGMENT_SHADER):
            return VK_SHADER_STAGE_FRAGMENT_BIT;
    }
}

VkPipelineShaderStageCreateInfo makeShaderCreateInfo(render::EShaderType type,
                                                     const VkShaderModule& module)
{
    return [&]
    {
        VkPipelineShaderStageCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ci.stage = getVulkanStageType(type);
        ci.module = module;
        ci.pName = "main"; // shader entrypoint.

        return ci;
    }();
}

} // anonymous namespace

namespace render
{

Shader::Shader(VkDevice device, const std::string& path, EShaderType type)
    : device(device)
{
    auto vShaderCode = utils::readFileBinary(path);
    //shaderModule = createShaderModule(vShaderCode, device);
    // This pointer with custom deleter is wholly responsible for
    // VkShaderModule lifetime. This allows copy constructor to work
    // effectively.
    shaderModule = std::shared_ptr<VkShaderModule>
        {new VkShaderModule(createShaderModule(vShaderCode, device)),
            [device](VkShaderModule* p)
            {
                vkDestroyShaderModule(device, *p, nullptr);
                delete p;
            }};

    createInfo = makeShaderCreateInfo(type, *shaderModule);
}

Shader::Shader(Shader&& rhs)
    : shaderModule(std::move(rhs.shaderModule))
    , createInfo(rhs.createInfo)
    , device(rhs.device)
{
    //rhs.shaderModule = VK_NULL_HANDLE;
    rhs.shaderModule.reset();
    rhs.device = VK_NULL_HANDLE;
    rhs.createInfo = decltype(rhs.createInfo){};
}

Shader::Shader(const Shader& rhs)
    : shaderModule(rhs.shaderModule)
    , createInfo(rhs.createInfo)
    , device(rhs.device)
{
}

Shader::~Shader()
{
    if(shaderModule != VK_NULL_HANDLE)
    {
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

} // namespace render
