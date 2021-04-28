#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <memory>


namespace render
{
enum class EShaderType
{
    VERTEX_SHADER,
    FRAGMENT_SHADER,
};

class Shader
{
public:
    Shader(VkDevice, const std::string& path, EShaderType);
    Shader(){};
    Shader(Shader&&);
    Shader(const Shader&);
    ~Shader();
    VkPipelineShaderStageCreateInfo getCi() const;
    VkShaderModule getShaderModule() const;
    VkDevice getDevice() const;


// @TODO: Zmienic moze shader module na std::shared_ptr z customowym deleterem?
// Umozliwi to copy ctory.
private:
    VkPipelineShaderStageCreateInfo createInfo{};
    std::shared_ptr<VkShaderModule> shaderModule{nullptr};
    VkDevice device{VK_NULL_HANDLE}; // sadly necessary to create shader.
    // this is prime dependency injection candidate
    // boost::di?
};
} // naemspace render
