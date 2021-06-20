#include "VulkanDevice.hpp"
#include "VulkanMacros.hpp"
#include "Logger.hpp"
#include <optional>
#include <set>
#include <climits>
#include <cassert>

namespace
{
std::optional<uint32_t> queryGraphicsFamilyIndice(VkPhysicalDevice device)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies{queueFamilyCount};
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (auto&& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            return i;
        }
        ++i;
    }

    return {};
}

render::QueueFamiliesIndices queryQueueFamilies(const VkPhysicalDevice& device, VkSurfaceKHR surface)
{
    render::QueueFamiliesIndices indices;

    indices.graphicsFamily = queryGraphicsFamilyIndice(device);

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies{queueFamilyCount};
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for(uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if(presentSupport)
        {
            indices.presentationFamily = i;
            return indices;
        }
        ++i;
    }

    return indices;
}

VkPhysicalDevice pickPhysicalDevice(const VkInstance& instance)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> physicalDevices{deviceCount};
    vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

    if (deviceCount == 0)
        throw std::runtime_error("No Vulkan supporting physical devices.");

    if(queryGraphicsFamilyIndice(physicalDevices[0]).has_value())
    {
        return physicalDevices[0];
    }
    else
    {
        return physicalDevices[1];
    }

    throw std::runtime_error("No device with all required queue families found.");
}

VkDevice createLogicalDevice(const VkPhysicalDevice& physicalDevice,
                            render::QueueFamiliesIndices indices)
{
    std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;
    std::set<uint32_t> uniqueQueueFamiliesIndices =
        {
            indices.graphicsFamily.value(),
            indices.presentationFamily.value(),
        };

    for(const auto& family : uniqueQueueFamiliesIndices)
    {
        float queuePriority = 1.0f;
        deviceQueueCreateInfos.push_back([&physicalDevice, &queuePriority, family]
            {
                VkDeviceQueueCreateInfo queueCreateInfo{};

                queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo.queueFamilyIndex = family;
                queueCreateInfo.queueCount = 1;
                queueCreateInfo.pQueuePriorities = &queuePriority;

                return queueCreateInfo;
            }());
    }

    // Right now we are not interested in any special features, so we enable none.
    VkPhysicalDeviceFeatures deviceFeatures{};
    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        //"VK_KHR_get_memory_requirements2",
        //"VK_KHR_dedicated_allocation"
    };

    const auto createInfo = [&deviceFeatures, &deviceQueueCreateInfos, &deviceExtensions]{
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
        createInfo.queueCreateInfoCount = deviceQueueCreateInfos.size();
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = deviceExtensions.size();
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        return createInfo;
    }();

    VkDevice device{};
    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    return device;
}

} // anon namespace

namespace render
{

VkResult VulkanDevice::createVkBuffer(
    VkBufferUsageFlags usage,
    VkDeviceSize size,
    VkBuffer* buffer)
{
    const auto buffer_ci = [usage, size]()
        {
            VkBufferCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            ci.usage = usage;
            ci.size = size;
            ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            return ci;
        }();

    return vkCreateBuffer(vkLogicalDevice, &buffer_ci, nullptr, buffer);
}

VkResult VulkanDevice::allocateVkDeviceMemory(
    VkMemoryPropertyFlags mem_prop_flags,
    VkBuffer buffer,
    VkDeviceMemory* memory,
    VkDeviceSize* alignment)
{
    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(vkLogicalDevice, buffer, &mem_reqs);

    auto mem_alloc_info = [this, &mem_reqs, mem_prop_flags]()
    	{
			VkMemoryAllocateInfo mai{};
			mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			mai.allocationSize = mem_reqs.size;
            mai.memoryTypeIndex = deviceUtils::getMemoryTypeIndex(
                    getPhysicalDevice(), mem_reqs.memoryTypeBits, mem_prop_flags);
			return mai;
		}();

    auto ret = vkAllocateMemory(vkLogicalDevice, &mem_alloc_info, nullptr, memory);

    if(alignment and (ret == VK_SUCCESS))
        *alignment = mem_reqs.alignment;

    return ret;
}

VulkanDevice::VulkanDevice(VkInstance instance, VkSurfaceKHR surface)
	: vkPhysicalDevice(pickPhysicalDevice(instance))
	, queueIndices(queryQueueFamilies(vkPhysicalDevice, surface))
	, vkLogicalDevice(createLogicalDevice(vkPhysicalDevice, queueIndices))
{
    vkGetPhysicalDeviceProperties(vkPhysicalDevice, &deviceProperties);
    vkGetPhysicalDeviceFeatures(vkPhysicalDevice, &deviceFeatures);
    vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &deviceMemProperties);

    vkGetDeviceQueue(vkLogicalDevice, getGraphicsQueueIndice(), 0, &graphicsQueue);
    vkGetDeviceQueue(vkLogicalDevice, getPresentationQueueIndice(), 0, &presentationQueue);
}

VulkanDevice::~VulkanDevice()
{
    //if(vkLogicalDevice != VK_NULL_HANDLE)
    //    vkDestroyDevice(vkLogicalDevice, nullptr);
}

namespace deviceUtils
{

uint32_t getMemoryTypeIndex(
        VkPhysicalDevice vkPhysicalDevice,
        uint32_t memTypeBits,
        VkMemoryPropertyFlags properties,
        VkBool32* success)
{
    VkPhysicalDeviceMemoryProperties deviceMemProperties;
    vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &deviceMemProperties);

    for (uint32_t i = 0; i < deviceMemProperties.memoryTypeCount; i++)
    {

        if ((memTypeBits & (1 << i)) &&
            (deviceMemProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            if(success)
            {
                *success = VK_TRUE;
            }
            return i;
        }
    }

    if(success)
    {
        *success = VK_FALSE;
        return UINT_MAX;
    }
    else
    {
        throw std::runtime_error("Cannot find suitable memory type");
    }
}

VkMemoryPropertyFlags getMemoryProperties(VkPhysicalDevice physDevice, uint32_t memIndex)
{
    assert(memIndex <= VK_MAX_MEMORY_TYPES);
    VkPhysicalDeviceMemoryProperties deviceMemProperties;
    vkGetPhysicalDeviceMemoryProperties(physDevice, &deviceMemProperties);

    return deviceMemProperties.memoryTypes[memIndex].propertyFlags;
}

}

} // namespace render
