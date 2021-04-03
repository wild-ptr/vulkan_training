#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>
#include <set>
#include <cstring>
#include <iostream>
#include <optional>
#include <cstdint>
#include <algorithm>
#include <thread>
#include <fstream>

#include "VulkanApplication.hpp"
#include "Logger.hpp"

namespace
{

#ifdef NDEBUG
	static constexpr bool enableValidationLayers = false;
#else
	static constexpr bool enableValidationLayers = true;
#endif

std::vector<const char*> getRequiredExtensions(bool enableValidationLayers)
{
	uint32_t glfwExtensionsCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);

	std::vector<const char*> extensions{glfwExtensions, glfwExtensions + glfwExtensionsCount};

	if (enableValidationLayers)
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); // this macro expands to string.

	return extensions;
}

bool checkValidationLayerSupport(const std::vector<const char*>& validationLayersRequested)
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers{layerCount};
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const auto& layerProperties : availableLayers)
	{
	    for (const auto& layer : validationLayersRequested)
		{
			if(strcmp(layer, layerProperties.layerName) == 0)
				return true; // right now this only checks one layer, and not a vector.
		}
	}

	return false;
}

VkInstance createInstance()
{
	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	if(enableValidationLayers and not checkValidationLayerSupport(validationLayers))
	{
		throw std::runtime_error("validation layers requested, not available.");
	}

	const auto appInfo = []{
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello, Vulkan!";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Riverfish";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;
		return appInfo;
	}();

	auto extensions = getRequiredExtensions(enableValidationLayers);
	const auto createInfo = [&appInfo, &validationLayers, &extensions]
	{
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = std::addressof(appInfo);

		createInfo.enabledExtensionCount = extensions.size();
		createInfo.ppEnabledExtensionNames = extensions.data();

		if(enableValidationLayers)
		{
			createInfo.enabledLayerCount = validationLayers.size();
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		return createInfo;
	}();

	VkInstance vkInstance;
	if (vkCreateInstance(&createInfo, nullptr, &vkInstance) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create Vulkan instance.");
	}

	return vkInstance;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugLayerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    dbgValidLayer << pCallbackData->pMessage << NEWL;

    return VK_FALSE;
}

// Since debug utils is an extension, we have to manually load address to create/destroy debug messenger
VkResult CreateDebugUtilsMessengerEXT(
		VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
                                                        instance,
                                                        "vkCreateDebugUtilsMessengerEXT");

    if (func != nullptr)
	{
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
	else
	{
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(
		VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
                                                        instance,
                                                        "vkDestroyDebugUtilsMessengerEXT");

    if (func != nullptr)
	{
        func(instance, debugMessenger, pAllocator);
    }
}

VkDebugUtilsMessengerEXT setupDebugMessenger(VkInstance instance)
{
	if (!enableValidationLayers)
		return VkDebugUtilsMessengerEXT{};

	const auto createInfo = []()
	{
		VkDebugUtilsMessengerCreateInfoEXT createInfo{};

		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

		createInfo.messageSeverity =
		    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

		createInfo.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

		createInfo.pfnUserCallback = debugLayerCallback;
		createInfo.pUserData = nullptr; // Optional
		return createInfo;
	}();

	VkDebugUtilsMessengerEXT debugMessenger;

	if(CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
		throw std::runtime_error("failed to set up debug messenger");

	return debugMessenger;
}

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

vk_main::QueueFamiliesIndices queryQueueFamilies(const VkPhysicalDevice& device, VkSurfaceKHR surface)
{
    vk_main::QueueFamiliesIndices indices;

    indices.graphicsFamily = queryGraphicsFamilyIndice(device);

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies{queueFamilyCount};
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for(int i = 0; i < queueFamilyCount; ++i)
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

    // take first applicable
    for (auto&& device : physicalDevices)
    {
        if (queryGraphicsFamilyIndice(device).has_value())
            return device;
    }

    throw std::runtime_error("No device with all required queue families found.");
}

VkDevice createLogicalDevice(const VkPhysicalDevice& physicalDevice,
                            vk_main::QueueFamiliesIndices indices)
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
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
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

vk_main::SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    vk_main::SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if(formatCount)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if(presentModeCount)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount,
                details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
    for (auto&& format : formats)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return format;
    }

    return formats[0]; // fallback, should not happen.
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes)
{
    for (auto&& presentMode : presentModes)
    {
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return presentMode;
    }

    return VK_PRESENT_MODE_FIFO_KHR; // guaranteed availability.
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window)
{

    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        return capabilities.currentExtent;
    }
    else
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        // clamp between minimum and maximum extents supported by implementation
        // seems like unnecessary step in 99.9% of cases.
        actualExtent.width = std::max(capabilities.minImageExtent.width,
                                      std::min(capabilities.maxImageExtent.width,
                                               actualExtent.width));

        actualExtent.height = std::max(capabilities.minImageExtent.width,
                                      std::min(capabilities.maxImageExtent.height,
                                               actualExtent.height));

        return actualExtent;
    }
}

std::vector<char> readFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if(!file.is_open())
        throw std::runtime_error("cannot open shader file? Wrong path?");

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

} // anonymous namespace

namespace vk_main
{

void VulkanApplication::run()
{
	initWindow();
	initVulkan();
	mainLoop();
	cleanup();
}

void VulkanApplication::createSurface()
{
    if (glfwCreateWindowSurface(vkInstance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

void VulkanApplication::initWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

void VulkanApplication::createSwapChain()
{
    glfwPollEvents();
    auto swapChainSupport = querySwapChainSupport(vkPhysicalDevice, surface);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if(swapChainSupport.capabilities.maxImageCount > 0 and
        imageCount > swapChainSupport.capabilities.maxImageCount)
        imageCount = swapChainSupport.capabilities.maxImageCount;

    const auto createInfo = [&, this]()
        {
            VkSwapchainCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            createInfo.surface = surface;

            createInfo.minImageCount = imageCount;
            createInfo.imageFormat = surfaceFormat.format;
            createInfo.imageColorSpace = surfaceFormat.colorSpace;
            createInfo.imageExtent = extent;
            createInfo.imageArrayLayers = 1;
            createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            auto indices = queryQueueFamilies(vkPhysicalDevice, surface);

            uint32_t queueFamilyIndices[] = {
                indices.graphicsFamily.value(),
                indices.presentationFamily.value()
            };

            if (indices.graphicsFamily != indices.presentationFamily)
            {
                createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                createInfo.queueFamilyIndexCount = 2;
                createInfo.pQueueFamilyIndices = queueFamilyIndices;
            }
            else
            {
                createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            }

            // we dont want any transformations.
            createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

            // blending with other windows in window system, we want to ignore alpha channel.
            createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

            createInfo.presentMode = presentMode;
            createInfo.clipped = VK_TRUE; // we dont care about pixels that are obscured by oth windows
            createInfo.oldSwapchain = VK_NULL_HANDLE;

            return createInfo;
        }();

    dbgI << "attempting to create swapchain." << NEWL;

    if (vkCreateSwapchainKHR(vkLogicalDevice, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
        throw std::runtime_error("Failed to create swapchain.");

    uint32_t imagesCount;
    vkGetSwapchainImagesKHR(vkLogicalDevice, swapChain, &imagesCount, nullptr);
    swapChainImages.reserve(imagesCount);
    vkGetSwapchainImagesKHR(vkLogicalDevice, swapChain, &imagesCount, swapChainImages.data());

}

void VulkanApplication::createImageViews()
{
    dbgI << "Creating image views." << NEWL;

    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); ++i)
    {
        const auto createInfo = [&, this]
        {
            VkImageViewCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            ci.image = swapChainImages[i];
            ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
            ci.format = swapChainImageFormat;

            ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            ci.subresourceRange.baseMipLevel = 0;
            ci.subresourceRange.levelCount = 1;
            ci.subresourceRange.baseArrayLayer = 0;
            ci.subresourceRange.layerCount = 1;

            return ci;
        }();

        if (vkCreateImageView(vkLogicalDevice, &createInfo, nullptr, &swapChainImageViews[i])
                != VK_SUCCESS)
            throw std::runtime_error("Failed to create swapchain.");
    }
}

VkShaderModule VulkanApplication::createShaderModule(const std::vector<char>& code)
{
    const auto createInfo = [&code, this]()
    {
        VkShaderModuleCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        ci.codeSize = code.size();
        ci.pCode = reinterpret_cast<const uint32_t*>(code.data());
        return ci;
    }();

    VkShaderModule shaderModule;

    if(vkCreateShaderModule(vkLogicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        throw std::runtime_error("Cannot create shadermodule from code.");

    return shaderModule;
}

void VulkanApplication::createGraphicsPipeline()
{
    auto vertShaderCode = readFile("shaders/vert.spv");
    auto fragShaderCode = readFile("shaders/frag.spv");

    auto vertModule = createShaderModule(vertShaderCode);
    auto fragModule = createShaderModule(fragShaderCode);

    const auto vertexStageCi = [&, this]
    {
        VkPipelineShaderStageCreateInfo ci;
        ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ci.stage = VK_SHADER_STAGE_VERTEX_BIT;
        ci.module = vertModule;
        ci.pName = "main"; // shader entrypoint.

        return ci;
    }();

    const auto fragmentStageCi = [&, this]
    {
        VkPipelineShaderStageCreateInfo ci;
        ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ci.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        ci.module = fragModule;
        ci.pName = "main"; // shader entrypoint.

        return ci;
    }();

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertexStageCi, fragmentStageCi};

    // Shader modules can be destroyed as shader stage will copy them.
    // All of this will be RAII wrapped in the future.
    vkDestroyShaderModule(vkLogicalDevice, vertModule, nullptr);
    vkDestroyShaderModule(vkLogicalDevice, fragModule, nullptr);
}

void VulkanApplication::initVulkan()
{
	vkInstance = createInstance();
	debugMessenger = setupDebugMessenger(vkInstance);
    createSurface();
    vkPhysicalDevice = pickPhysicalDevice(vkInstance);
    indices = queryQueueFamilies(vkPhysicalDevice, surface);
    vkLogicalDevice = createLogicalDevice(vkPhysicalDevice, indices);

    vkGetDeviceQueue(vkLogicalDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(vkLogicalDevice, indices.presentationFamily.value(), 0, &presentationQueue);

    createSwapChain();
    createImageViews();

    createGraphicsPipeline();
}

void VulkanApplication::mainLoop()
{
	while (not glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}
}

void VulkanApplication::cleanup()
{
	if(enableValidationLayers)
	{
		DestroyDebugUtilsMessengerEXT(vkInstance, debugMessenger, nullptr);
	}

    for (auto&& image : swapChainImageViews)
        vkDestroyImageView(vkLogicalDevice, image, nullptr);

    vkDestroySwapchainKHR(vkLogicalDevice, swapChain, nullptr);
    vkDestroySurfaceKHR(vkInstance, surface, nullptr);
    vkDestroyDevice(vkLogicalDevice, nullptr);
	vkDestroyInstance(vkInstance, nullptr);
	glfwDestroyWindow(window);
	glfwTerminate();
}


} // namespace vk_main
