#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>
#include <set>
#include <cstring>
#include <iostream>
#include <optional>

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
    std::cout << "start of queryGraphicsFamilyIndice" << std::endl;
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
    std::cout << "do we get here?" << std::endl;
    vk_main::QueueFamiliesIndices indices;

    indices.graphicsFamily = queryGraphicsFamilyIndice(device);
    std::cout << "do we get here?" << std::endl;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies{queueFamilyCount};
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for(int i = 0; i<queueFamilyCount; ++i)
    {
        std::cout << "do we get here?" << std::endl;
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

    const auto createInfo = [&deviceFeatures, &deviceQueueCreateInfos]{
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
        createInfo.queueCreateInfoCount = deviceQueueCreateInfos.size();
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = 0;

        return createInfo;
    }();

    VkDevice device{};
    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    return device;
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

    vkDestroySurfaceKHR(vkInstance, surface, nullptr);
    vkDestroyDevice(vkLogicalDevice, nullptr);
	vkDestroyInstance(vkInstance, nullptr);
	glfwDestroyWindow(window);
	glfwTerminate();
}


} // namespace vk_main
