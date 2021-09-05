#include "TextureManager.hpp"
#include "Logger.hpp"
#include "stb_image.h"
#include <cassert>
#include "Constants.hpp"

namespace render::memory
{

namespace
{
struct image_data
{
    image_data(const std::string& path)
    {
	    image = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        channels = 4; // we force rgba textures. For RGB textures A is forced to 255

        size = width * height * channels;

        if(image)
        {
            valid = true;
        }
    }

    ~image_data()
    {
        if(image and valid)
        {
            stbi_image_free(image);
        }
    };

    bool isValid()
    {
        return valid;
    }

    stbi_uc* image{nullptr};
    size_t size;
    int width, height, channels;
    bool valid{false};
};

} // anonymous namespace

TextureManager::TextureManager(std::shared_ptr<VulkanDevice> device_ptr)
    : device(std::move(device_ptr))
{
    createPlaceholderImage();
    createSampler();
    initialBindingInformationCreation();
}

void TextureManager::initialBindingInformationCreation()
{
    assert(placeholder_image);

    binding_info.poolSizes =
    {
        {
            .type = VK_DESCRIPTOR_TYPE_SAMPLER,
            .descriptorCount = consts::maxFramesInFlight,
        },
        {
            .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount = consts::maxFramesInFlight * TEXTURES_MAX,
        }
    };

    for (uint32_t i = 0; i < TEXTURES_MAX; ++i)
    {
        binding_info.descriptors[i].sampler = nullptr;
        binding_info.descriptors[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        binding_info.descriptors[i].imageView = placeholder_image->getImageView();
    }

    binding_info.samplerDescriptor.sampler = sampler;
    binding_info.samplerDescriptor.imageView = VK_NULL_HANDLE;
}

// a yellow - red stripped texture will do.
void TextureManager::createPlaceholderImage()
{
    dbgI << "Creating placeholder image for out-of-bounds texture accesses..." << NEWL;

    struct __attribute__((packed)) Pixel
    {
        uint8_t r,g,b,a;
    };
    constexpr size_t dimensions = 500;
    std::array<Pixel, dimensions * dimensions> texture_data;

    for(auto& pixel : texture_data)
    {
        pixel.r = 255;
        pixel.g = 255;
        pixel.b = 0;
        pixel.a = 255;
    }

    for(size_t i = 0; i < dimensions * dimensions; ++i)
    {
        if(i % 2 == 0)
        texture_data[i].g = 0;
    }

    VulkanImageCreateInfo ci =
    {
        .width = dimensions,
        .height = dimensions,
        .layerCount = 1,
        .mipLevels = 1,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    };

    // at this point the image is ready to be used and properly transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    placeholder_image = std::make_unique<VulkanImage>(ci, device, texture_data.data(), texture_data.size() * sizeof(Pixel));
    dbgI << "placeholder image properly created and transitioned!" << NEWL;
}

void TextureManager::createSampler()
{
    VkSamplerCreateInfo ci = [](VkFilter filters, VkSamplerAddressMode samplerAddressMode)
    {
        VkSamplerCreateInfo info{};
	    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	    info.pNext = nullptr;

	    info.magFilter = filters;
	    info.minFilter = filters;
	    info.addressModeU = samplerAddressMode;
	    info.addressModeV = samplerAddressMode;
	    info.addressModeW = samplerAddressMode;

        info.anisotropyEnable = VK_TRUE;
        info.maxAnisotropy = 16.0;
	    return info;
    }(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);

    vkCreateSampler(device->getDevice(), &ci, nullptr, &sampler);
}

size_t TextureManager::loadTexture(const std::string& path)
{
    dbgI << "trying to load texture: " << path << NEWL;
    if(auto it = findInIndexMapSafe(path); it != index_map.end())
    {
        return it->second;
    }

    image_data image{path};
    if(not image.isValid())
    {
        dbgI << "Invalid image presented." << NEWL;
        return TEXTURES_MAX - 1;
    }

    VulkanImageCreateInfo ci =
    {
        .width = static_cast<uint32_t>(image.width),
        .height = static_cast<uint32_t>(image.height),
        .layerCount = 1,
        .mipLevels = 1,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    };

    auto texture = std::make_unique<VulkanImage>(ci, device, image.image, image.size);

    // array insertion and descriptor generation can be parallelized because every load will touch different index number.
    // This will be false-shared sometimes because of cache-line occupancy. This is non-realtime for now so fuck it.
    // seq-cst as every thread needs to know about current index. Im not sure i can get away with acq-rel ordering here.
    size_t texture_index = num_of_textures.fetch_add(1, std::memory_order_seq_cst);
    if(texture_index >= TEXTURES_MAX)
    {
        dbgE << "Trying to create texture over maximum. Increase texture limits. Aborting exec." << NEWL;
        throw std::runtime_error("Texture limit reached!");
    }

    textures[texture_index] = std::move(texture);
    setInIndexMapSafe(path, texture_index);
    generateDescriptorEntry(texture_index);

    dbgI << "Proper texture created." << NEWL;

    return texture_index;
}

std::map<std::string, size_t>::iterator TextureManager::findInIndexMapSafe(const std::string& key)
{
    std::shared_lock lock(index_map_mut);
    return index_map.find(key);
}

void TextureManager::setInIndexMapSafe(const std::string& key, size_t value)
{
    std::unique_lock lock(index_map_mut);
    index_map[key] = value;
}

void TextureManager::generateDescriptorEntry(size_t texture_index)
{
    assert(texture_index < TEXTURES_MAX);
    binding_info.descriptors[texture_index].imageView = textures[texture_index]->getImageView();
}

// Unsafe, i should just mutex it all. Torn reads are possible here because we can be mangling
// with descriptors while doing vkUpdateDescriptorSets. Too bad!
void TextureManager::fillDescriptorSet(VkDescriptorSet descriptorSet)
{
    // @TODO: This should all be done as one call to UpdateDescriptorSets.
    VkWriteDescriptorSet wds_tex_array = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptorSet,
        .dstBinding = consts::perFrame_textureArrayBinding,
        .dstArrayElement = 0,
        .descriptorCount = TEXTURES_MAX,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .pImageInfo = binding_info.descriptors.data(),
    };

    vkUpdateDescriptorSets(device->getDevice(), 1, &wds_tex_array, 0, nullptr);

    VkWriteDescriptorSet wds_sampler = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptorSet,
        .dstBinding = consts::perFrame_textureSamplerBinding,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
        .pImageInfo = &binding_info.samplerDescriptor,
    };

    vkUpdateDescriptorSets(device->getDevice(), 1, &wds_sampler, 0, nullptr);
}

} // namespace render::memory
