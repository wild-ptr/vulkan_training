#include "TextureManager.hpp"
#include "Logger.hpp"
#include "stb_image.h"
#include <cassert>

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

        // I think stbi_image does it by itself.
        //for(size_t i = 0; i < size; i += 4)
        //{
        //    image[i + 3] = 255;
        //}

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

    // this should be removed as we will get this from reflection data.
    binding_info.setLayoutBindingTextureArray =
    {
        .binding = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .descriptorCount = TEXTURES_MAX,
        .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS,
        .pImmutableSamplers = nullptr,
    };

    binding_info.setLayoutBindingSampler =
    {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS,
        .pImmutableSamplers = nullptr,
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

// a plain yellow texture will do.
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

	    return info;
    }(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT);

    vkCreateSampler(device->getDevice(), &ci, nullptr, &sampler);
}

size_t TextureManager::loadTexture(const std::string& path)
{
    if(auto it = findInIndexMapSafe(path); it != index_map.end())
    {
        return it->second;
    }

    image_data image{path};
    if(not image.isValid())
    {
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

} // namespace render::memory
