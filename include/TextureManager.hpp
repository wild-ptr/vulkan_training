// This will be used to load textures and provide indices for textures.
#pragma once
#include "VulkanDevice.hpp"
#include "VulkanImage.hpp"

#include <memory>
#include <map>
#include <array>
#include <atomic>
#include <shared_mutex>

namespace render::memory
{

// This class shall only provide descriptors, and not whole uniform buffer object,
// as it is meant to be used as part of per-frame rebind frequency system.
// Indices of textures can be passed in as push constants on per-object basis.

// Arbitrary limit, can be easily extended in the future. But it has to be there
// as we need a hard limit on texture array in our shaders.
constexpr size_t TEXTURES_MAX = 4096;

// will be used to write to per-frame descriptor set.
// @TODO: Bring the descriptors and samplerDescriptor into private members as they are not necessary.
// Make a generic BindingInformation struct along with IPerFrameSystem interface that will be enforced.

struct BindingInformationTextures
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    std::array<VkDescriptorImageInfo, TEXTURES_MAX> descriptors{};
    VkDescriptorImageInfo samplerDescriptor{};
};

class TextureManager
{
public:
    TextureManager(std::shared_ptr<VulkanDevice> device);

    // returns texture indice. Unloading shall not be supported for now.
    // If already loaded, get indice.
    size_t loadTexture(const std::string& path);
    const BindingInformationTextures& getBindingInformation() { return binding_info; }
    void fillDescriptorSet(VkDescriptorSet);


private:
    void createPlaceholderImage();
    void createSampler();
    void initialBindingInformationCreation();
    void generateDescriptorEntry(size_t texture_index);

    std::map<std::string, size_t>::iterator findInIndexMapSafe(const std::string& key);
    void setInIndexMapSafe(const std::string& key, size_t value);

    std::shared_ptr<VulkanDevice> device;
    std::unique_ptr<VulkanImage> placeholder_image;

    // I will make a real mt wrapper for map later and use that.
    std::atomic<size_t> num_of_textures{0};
    std::shared_mutex index_map_mut;
    std::map<std::string, size_t> index_map;
    std::array<std::unique_ptr<VulkanImage>, TEXTURES_MAX> textures;
    VkSampler sampler;

    BindingInformationTextures binding_info;
};
} // namespace render::memory
