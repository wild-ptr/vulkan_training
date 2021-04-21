#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>


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
    Shader(const Shader&) = delete;
    ~Shader();
    VkPipelineShaderStageCreateInfo getCi() const;
    VkShaderModule getShaderModule() const;


// @TODO: Zmienic moze shader module na std::shared_ptr z customowym deleterem?
// Umozliwi to copy ctory.
private:
    VkPipelineShaderStageCreateInfo createInfo{};
    VkShaderModule shaderModule{VK_NULL_HANDLE};
    VkDevice device{VK_NULL_HANDLE}; // sadly necessary to create shader.
    // this is prime dependency injection candidate
    // boost::di?
};
} // naemspace render
