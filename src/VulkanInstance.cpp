#include "VulkanInstance.hpp"
#include "Logger.hpp"
#include <vector>
#include <cstring>

namespace
{
// VkInstance creation utilities
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

bool checkValidationLayerSupport(const std::vector<const char*>& validationLayersEnabledRequested)
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers{layerCount};
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const auto& layerProperties : availableLayers)
	{
	    for (const auto& layer : validationLayersEnabledRequested)
		{
			if(strcmp(layer, layerProperties.layerName) == 0)
				return true; // right now this only checks one layer, and not a vector.
		}
	}

	return false;
}

VkInstance createInstance(bool enableValidationLayers)
{
	const std::vector<const char*> validationLayersEnabled = {
		"VK_LAYER_KHRONOS_validation"
	};

	if(enableValidationLayers and not checkValidationLayerSupport(validationLayersEnabled))
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
	const auto createInfo = [&appInfo, &validationLayersEnabled, &extensions, enableValidationLayers]
	{
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = std::addressof(appInfo);

		createInfo.enabledExtensionCount = extensions.size();
		createInfo.ppEnabledExtensionNames = extensions.data();

		if(enableValidationLayers)
		{
			createInfo.enabledLayerCount = validationLayersEnabled.size();
			createInfo.ppEnabledLayerNames = validationLayersEnabled.data();
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

// debug messenger utilities
static VKAPI_ATTR VkBool32 VKAPI_CALL debugLayerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
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

VkDebugUtilsMessengerEXT setupDebugMessenger(VkInstance instance, bool enableValidationLayers)
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
} // anon namespace


namespace render
{

// dummy to allow deferred creation.
VulkanInstance::VulkanInstance(){}

VulkanInstance::VulkanInstance(bool debugFlag)
	: validationLayersEnabled(debugFlag)
	, vkInstance(createInstance(validationLayersEnabled))
{
	if(validationLayersEnabled)
		debugMessenger = setupDebugMessenger(vkInstance, validationLayersEnabled);

	dbgI << "VkInstance created. Debug mode: " << std::boolalpha << validationLayersEnabled << NEWL;
	std::cout << "instance handle: " << (uint64_t)vkInstance << std::endl;
}

VulkanInstance::~VulkanInstance()
{
	//if(validationLayersEnabled)
	//	DestroyDebugUtilsMessengerEXT(vkInstance, debugMessenger, nullptr);
    //
	//vkDestroyInstance(vkInstance, nullptr);
}

} // namespace render
