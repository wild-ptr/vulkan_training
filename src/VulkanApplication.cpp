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
#include <string.h>

#include "VulkanApplication.hpp"
#include "Logger.hpp"
#include "Shader.hpp"
#include "Vertex.hpp"

namespace
{

#ifdef NDEBUG
	static constexpr bool enableValidationLayers = false;
#else
	static constexpr bool enableValidationLayers = true;
#endif
    static constexpr auto maxFramesInFlight = 2u;

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

render::QueueFamiliesIndices queryQueueFamilies(const VkPhysicalDevice& device, VkSurfaceKHR surface)
{
    render::QueueFamiliesIndices indices;

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

    if(queryGraphicsFamilyIndice(physicalDevices[0]).has_value())
    {
        dbgI << "Device 0, probably Intel HD Graphics." << NEWL;
        return physicalDevices[0];
    }
    else
    {
        return physicalDevices[1];
    }

    // take first applicable geforce xD
   // for (auto&& device : physicalDevices)
   // {
   //     VkPhysicalDeviceProperties props;
   //     vkGetPhysicalDeviceProperties(device, &props);

   //     const char* wantedVendor = "GeForce";

   //     if (queryGraphicsFamilyIndice(device).has_value() &&
   //             strstr(props.deviceName, wantedVendor) != nullptr)
   //         return device;
   // }

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

render::SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    render::SwapChainSupportDetails details;

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

namespace render
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

    // members initialization as we need that data later.
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
            // Otherwise our application would become semi-transparent except of what we are rendering
            createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

            createInfo.presentMode = presentMode;
            createInfo.clipped = VK_TRUE; // we dont care about pixels that are obscured by oth windows
            createInfo.oldSwapchain = VK_NULL_HANDLE;

            return createInfo;
        }();

    dbgI << "attempting to create swapchain." << NEWL;

    if (vkCreateSwapchainKHR(vkLogicalDevice, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
        throw std::runtime_error("Failed to create swapchain.");

    // And there we get vkImage handles from swapchain.
    uint32_t imagesCount;
    vkGetSwapchainImagesKHR(vkLogicalDevice, swapChain, &imagesCount, nullptr);
    swapChainImages.resize(imagesCount);
    vkGetSwapchainImagesKHR(vkLogicalDevice, swapChain, &imagesCount, swapChainImages.data());

}

void VulkanApplication::createImageViews()
{
    dbgI << "Creating image views." << NEWL;
    dbgI << "swapChainImages size: " << swapChainImages.size() << NEWL;

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
    std::vector<Shader> shaders = {
        Shader{vkLogicalDevice, "shaders/vert.spv", EShaderType::VERTEX_SHADER},
        Shader{vkLogicalDevice, "shaders/frag.spv", EShaderType::FRAGMENT_SHADER}
    };

    pipeline = Pipeline{shaders, swapChainExtent, vkLogicalDevice, renderPass};
}

void VulkanApplication::createRenderPass()
{
    // so this is kinda our render target, we render to color buffer.
    const auto colorAttachment = [this]
    {
        VkAttachmentDescription ca{}; // DESCRIPTION
        ca.format = swapChainImageFormat;
        ca.samples = VK_SAMPLE_COUNT_1_BIT; // this is used for MSAA.

        ca.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // reset background to black per frame.
        ca.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // store results to be read later to present.

        ca.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        ca.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        ca.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // dont care about previous image layout.
        ca.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // ready for presentation after render.

        return ca;
    }();

    // we will create a simple, one subpass run.

    // attachments references are used by subpasses,
    // so we specify that for subpasses using this reference, we want image
    // to transition to color optimal layout before rendering using the given subpass.
    const auto colorAttachmentRef = []
    {
        VkAttachmentReference car{}; // REFERENCE TO ATTACHMENT
        car.attachment = 0;
        car.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        return car;
    }(); // so this subpass is rendering to color attachment. Easy enough.

    // Now this is important, we bind fragment shader output to render target there.
    const auto subpass = [&colorAttachmentRef]
    {
        VkSubpassDescription sp{};
        sp.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        sp.colorAttachmentCount = 1;
        sp.pColorAttachments = &colorAttachmentRef; // location 0 in our fragshader, since its [1] array

        return sp;
    }();

    const auto subpassDependency = [this]
    {
        VkSubpassDependency sd{};
        sd.srcSubpass = VK_SUBPASS_EXTERNAL; // implicit subpass before dstSubpass.
        sd.dstSubpass = 0; // index of subpass

        // The renderpass has two implicit stages, one at the end of the render pass
        // to transition image into presentation layout, and one at the beginning to
        // transition newly acquired image to beginning layout.
        // The renderpass will do this in the very moment we start our pipeline,
        // and this is too early, as we still havent acquired an image at this point.
        // So we tell renderpass to wait with transitions untill we get to fragment shader,
        // as then (as we specified with semaphores during submission) image must be available to us.
        // src - wait on this stage to happen
        sd.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        sd.srcAccessMask = 0;

        // Color attachment write operation in color attachment stage
        // should wait for transition before writing.
        // what should wait on src - in our case attachment writing should wait.
        sd.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        sd.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        return sd;
    }();

    const auto renderPassInfo = [&colorAttachment, &subpassDependency, &subpass]
    {
        VkRenderPassCreateInfo rpi{};
        rpi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rpi.attachmentCount = 1;
        rpi.pAttachments = &colorAttachment;
        rpi.subpassCount = 1;
        rpi.pSubpasses = &subpass;

        rpi.dependencyCount = 1;
        rpi.pDependencies = &subpassDependency;

        return rpi;
    }();

        dbgI << "attempting to create RenderPass" << NEWL;

    if (vkCreateRenderPass(vkLogicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
        throw std::runtime_error("Cannot create render pass!");

    dbgI << "created renderpass!" << NEWL;
}

// We can actually plug in multiple vkImages into the renderpass right there.
void VulkanApplication::createFramebuffers()
{
    dbgI << "swapChainImageViews size: " << swapChainImageViews.size() << NEWL;
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for(size_t i = 0; i < swapChainImageViews.size(); ++i)
    {
        VkImageView attachments[] = {
            swapChainImageViews[i]
        };

        const auto framebufferInfo = [&attachments, this]
        {
            VkFramebufferCreateInfo fi{};
            fi.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fi.renderPass = renderPass;
            fi.attachmentCount = 1;
            fi.pAttachments = attachments;
            fi.width = swapChainExtent.width;
            fi.height = swapChainExtent.height;
            fi.layers = 1;

            return fi;
        }();

        if (vkCreateFramebuffer(vkLogicalDevice, &framebufferInfo, nullptr, &swapChainFramebuffers[i])
            != VK_SUCCESS)
            throw std::runtime_error("Cannot create framebuffers.");
    }
}

void VulkanApplication::createCommandPool()
{
    const auto poolInfo = [this]
    {
        VkCommandPoolCreateInfo pi{};
        pi.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pi.queueFamilyIndex = indices.graphicsFamily.value();
        pi.flags = 0;
        return pi;
    }();

    if (vkCreateCommandPool(vkLogicalDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create command pool.");
}

void VulkanApplication::createCommandBuffers()
{
    dbgI << "swapChainFB size: " << swapChainFramebuffers.size() << NEWL;
    commandBuffers.resize(swapChainFramebuffers.size());

    const auto allocateInfo = [this]
    {
        VkCommandBufferAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ai.commandPool = commandPool;
        // Primary buffer - can be executed directly
        // Secondary - cannot be executed directly, but can be called from Primary buffers.
        ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandBufferCount = swapChainFramebuffers.size();

        return ai;
    }();

    if (vkAllocateCommandBuffers(vkLogicalDevice, &allocateInfo, commandBuffers.data()) != VK_SUCCESS)
        throw std::runtime_error("cannot allocate command buffers!");
}

void VulkanApplication::recordCommandBuffers()
{
    for(size_t i = 0; i < commandBuffers.size(); ++i)
    {
        const auto beginInfo = []
        {
            VkCommandBufferBeginInfo cbi{};
            cbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cbi.flags = 0;
            cbi.pInheritanceInfo = nullptr;

            return cbi;
        }();

        if(vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
            throw std::runtime_error("cannot begin command buffer.");

        VkClearValue clearValue = {0.0f, 0.0f, 0.0f, 1.0f};
        const auto renderPassInfo = [i, &clearValue, this]
        {
            VkRenderPassBeginInfo rbi{};
            rbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            rbi.renderPass = renderPass;
            rbi.framebuffer = swapChainFramebuffers[i];

            rbi.renderArea.offset = {0, 0};
            rbi.renderArea.extent = swapChainExtent;

            rbi.clearValueCount = 1;
            rbi.pClearValues = &clearValue;

            return rbi;
        }();

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline.getHandle());
        vkCmdDraw(commandBuffers[i], 3, 1, 0, 0); // 3 vertices, 1 instance,
                                                  // offset of vertex, offset of instance

        vkCmdEndRenderPass(commandBuffers[i]);

        if(vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("failed to record command buffer.");
    }

}

void VulkanApplication::createSyncObjects()
{
    imageAvailableSemaphores.resize(maxFramesInFlight);
    renderFinishedSemaphores.resize(maxFramesInFlight);

    inFlightFences.resize(maxFramesInFlight);
    imagesInFlight.resize(swapChainFramebuffers.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for(size_t i = 0; i < imageAvailableSemaphores.size(); ++i)
    {
        if (vkCreateSemaphore(vkLogicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i])
                != VK_SUCCESS or
        vkCreateSemaphore(vkLogicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i])
                != VK_SUCCESS or
        vkCreateFence(vkLogicalDevice, &fenceInfo, nullptr, &inFlightFences[i] ) != VK_SUCCESS)
            throw std::runtime_error("failed to create semaphores!");
    }
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

    createRenderPass();
    createGraphicsPipeline();
    dbgI << "after pipeline creation step!" << NEWL;

    createFramebuffers();

    createCommandPool();
    createCommandBuffers();
    recordCommandBuffers();

    createSyncObjects();
}

void VulkanApplication::mainLoop()
{
	while (not glfwWindowShouldClose(window))
	{
		glfwPollEvents();
        drawFrame();

        vkDeviceWaitIdle(vkLogicalDevice);
	}
}

void VulkanApplication::drawFrame()
{
    // we check whether we can actually start rendering another frame, we want to have
    // a maximum of maxFramesInFlight on queue.
    vkWaitForFences(vkLogicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(vkLogicalDevice,
                          swapChain,
                          UINT64_MAX, // timeout in ns, uint64_max disables timeout.
                          imageAvailableSemaphores[currentFrame],
                          VK_NULL_HANDLE, //fence, if applicable.
                          &imageIndex);

    // The first fence is not enough of a synchronization point, as the vkAcquireNextImageKHR
    // can actually give us frames out-of-order, or we can have less images in swapchain
    // than our maximum frames in flight setting. Therefore we also have to check if given
    // swapchain image is not used by another frame, and if so, we have to stall.
    if(imagesInFlight[imageIndex] != VK_NULL_HANDLE)
        vkWaitForFences(vkLogicalDevice, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    // The inFlightFences[currentFrame] is lit now.
    // So is imagesInFlight[imageIndex], as they are one and the same fence.

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame] };

    const auto submitInfo = [&waitSemaphores, &waitStages, &signalSemaphores, imageIndex, this]
    {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        // This tells us to wait on semaphore, and on what stages we want to wait for which semaphores.
        // waitStages and waitSemaphores indices are linked, so here we will wait on ImageAvailable
        // with pipeline stage that outputs to color attachment.
        // Vertex stage can proceed even before we get an image to draw on in this case.
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

        // those semaphores will be lit when command buffer execution finishes.
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        return submitInfo;
    }();

    // then we reset the fence, so we can stall
    vkResetFences(vkLogicalDevice, 1, &inFlightFences[currentFrame]);

    // dont we have a race condition there? Yes but this is not multithreaded.
    // easy.
    if(vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
        throw std::runtime_error("Cannot submit to queue");

    VkSwapchainKHR swapchains[] = {swapChain};

    const auto presentInfo = [&signalSemaphores, &swapchains, &imageIndex, this]
    {
        VkPresentInfoKHR pi{};
        pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        pi.waitSemaphoreCount = 1;
        pi.pWaitSemaphores = signalSemaphores;

        pi.swapchainCount = 1;
        pi.pSwapchains = swapchains;
        pi.pImageIndices = &imageIndex;

        return pi;
    }();

    vkQueuePresentKHR(presentationQueue, &presentInfo);

    currentFrame = (currentFrame + 1) % maxFramesInFlight;
}

void VulkanApplication::cleanup()
{
	if(enableValidationLayers)
	{
		DestroyDebugUtilsMessengerEXT(vkInstance, debugMessenger, nullptr);
	}

    for(size_t i = 0; i < imageAvailableSemaphores.size(); ++i)
    {
        vkDestroySemaphore(vkLogicalDevice, imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(vkLogicalDevice, renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(vkLogicalDevice, inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(vkLogicalDevice, commandPool, nullptr);

    for(auto&& framebuffer : swapChainFramebuffers)
        vkDestroyFramebuffer(vkLogicalDevice, framebuffer, nullptr);

    //vkDestroyPipelineLayout(vkLogicalDevice, pipelineLayout, nullptr);
    vkDestroyRenderPass(vkLogicalDevice, renderPass, nullptr);
    vkDestroyPipeline(vkLogicalDevice, graphicsPipeline, nullptr);

    for (auto&& image : swapChainImageViews)
        vkDestroyImageView(vkLogicalDevice, image, nullptr);

    vkDestroySwapchainKHR(vkLogicalDevice, swapChain, nullptr);
    vkDestroySurfaceKHR(vkInstance, surface, nullptr);
    vkDestroyDevice(vkLogicalDevice, nullptr);
	vkDestroyInstance(vkInstance, nullptr);
	glfwDestroyWindow(window);
	glfwTerminate();
}



} // namespace render

