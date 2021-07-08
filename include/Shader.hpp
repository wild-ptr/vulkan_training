#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <memory>
#include <string>
#include <vector>

namespace render {

enum class EShaderType {
    VERTEX_SHADER,
    FRAGMENT_SHADER,
};

struct DescriptorSetLayoutData {
    uint32_t set_number;
    VkDescriptorSetLayoutCreateInfo create_info;
    std::vector<VkDescriptorSetLayoutBinding> bindings;
};

class Shader {
public:
    Shader(VkDevice, const std::string& path, EShaderType);
    Shader() {};
    Shader(Shader&&);
    Shader(const Shader&) = default;
    ~Shader();
    VkPipelineShaderStageCreateInfo getCi() const;
    VkShaderModule getShaderModule() const;
    VkDevice getDevice() const;
    const std::vector<VkDescriptorSetLayout>& getReflectedDescriptorSetLayouts() const;
    VkShaderStageFlagBits getShaderType() const;

    // to be removed afterwards i think.
    const std::vector<DescriptorSetLayoutData>& getSetLayoutData() const;

private:
    VkPipelineShaderStageCreateInfo createInfo {};
    std::shared_ptr<VkShaderModule> shaderModule { nullptr };
    VkDevice device { VK_NULL_HANDLE }; // sadly necessary to create shader.
    VkShaderStageFlagBits shaderType;
    std::vector<DescriptorSetLayoutData> setLayoutData;
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
};

} // naemspace render
