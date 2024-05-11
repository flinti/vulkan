#include "Application.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fmt/core.h>
#include <fstream>
#include <ios>
#include <limits>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <set>
#include <sstream>


Application::Application(spdlog::logger &log, bool enableValidationLayers)
	: log(log),
	isValidationLayersEnabled(enableValidationLayers)
{

	initWindow();
	initVulkan();
}

Application::~Application()
{
	log.info("running application destructor...");
	cleanup();
}


void Application::run()
{
	mainLoop();
}

void Application::initWindow()
{
	log.info("initializing window...");
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

void Application::initVulkan()
{
	log.info("initializing vulkan...");
	createInstance();
	setupDebugMessenger();
	createSurface();
	findAndChooseDevice();
	createLogicalDevice();
	createSwapChain();
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createFramebuffers();
	createCommandPool();
	createCommandBuffer();
	createSyncObjects();
}

void Application::createSurface()
{
	log.info("creating surface...");
	VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
	if (result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("glfwCreateWindowSurface failed with code {}", (int32_t) result));
	}
}

void Application::setupDebugMessenger()
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	fillDebugMessengerCreateInfo(createInfo);

	auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) (vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
	if (vkCreateDebugUtilsMessengerEXT == nullptr) {
		throw std::runtime_error("addres of vkCreateDebugUtilsMessengerEXT could not be looked up!");
	}
	VkResult result = vkCreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger);
	if(result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("vkCreateDebugUtilsMessengerEXT failed with code {}", (int32_t) result));
	}
}

void Application::fillDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo)
{
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData = this;
}

bool Application::checkValidationLayersSupported()
{
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char *layer : requiredValidationLayers) {
		bool found = false;

		for (const auto &layerProp : availableLayers) {
			if (!strcmp(layer, layerProp.layerName)) {
				found = true;
				break;
			}
		}

		if (!found) {
			return false;
		}
	}

	return true;
}

void Application::createInstance()
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "None";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	// check validation layers & enable if applicable
	if(isValidationLayersEnabled) {
		log.info("Validation layers enabled. Checking layer support...");

		if(!checkValidationLayersSupported()) {
			throw std::runtime_error("The required validation layers are not available!");
		}
		
		createInfo.enabledLayerCount = static_cast<uint32_t>(requiredValidationLayers.size());
		createInfo.ppEnabledLayerNames = requiredValidationLayers.data();
	} else {
		log.info("Validation layers disabled");
		createInfo.enabledLayerCount = 0;
	}

	// get extension list
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	extensions.resize(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	{
		std::stringstream logLine;
		logLine << "Available Vulkan extensions:";
		for (auto ext : extensions) {
			logLine << "\n\t" << ext.extensionName << " v" << ext.specVersion;
		}
		log.info(logLine.str());
	}

	// request the required extensions
	uint32_t glfwRequiredExtensionsCount = 0;
	const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwRequiredExtensionsCount);
	if (glfwExtensions == nullptr) {
		throw std::runtime_error("glfwGetRequiredInstanceExtensions returned NULL");
	}

	std::vector<const char *> extensionsToEnable(glfwExtensions, glfwExtensions + glfwRequiredExtensionsCount);
	if(isValidationLayersEnabled) {
		extensionsToEnable.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	createInfo.enabledExtensionCount = extensionsToEnable.size();
	createInfo.ppEnabledExtensionNames = extensionsToEnable.data();

	{
		std::stringstream logLine;
		logLine << "Extensions to enable:";
		for (const auto &extension : extensionsToEnable) {
			logLine << "\n\t" << extension;
		}
		log.info(logLine.str());
	}

	// request debug messenger for instance creation and destruction, if applicable
	VkDebugUtilsMessengerCreateInfoEXT instanceCreationDebugMessengerCreateInfo{};
	if (isValidationLayersEnabled) {
		fillDebugMessengerCreateInfo(instanceCreationDebugMessengerCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &instanceCreationDebugMessengerCreateInfo;
	}

	// create vulkan instance
	log.info("Creating instance...");
	VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
	if (result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("vkCreateInstance failed with code {}", (int32_t)result));
	}
	log.info("Vulkan instance created.");
}


void Application::findAndChooseDevice()
{
	log.info("listing GPUs and choosing suitable ones");

	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if (!deviceCount) {
		throw std::runtime_error("No GPUs found!");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	// list devices and select last suitable device
	std::stringstream logLine;
	logLine << "GPUs found:";
	for (auto device : devices) {
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		QueueFamilyIndices queueFamilyIndices = findNeededQueueFamilyIndices(device);
		SwapChainSupportDetails swapChainSupportDetails = querySwapChainSupport(device);
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		bool suitable = isDeviceSuitable(device, queueFamilyIndices, swapChainSupportDetails, deviceProperties, deviceFeatures);
		if (suitable) {
			physicalDevice = device;
		}

		logLine << "\n\tID " << deviceProperties.deviceID << ": " << deviceProperties.deviceName;
	}
	log.info(logLine.str());

	if (physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("GPUs were found, but no device is suitable!");
	}

	log.info("suitable device chosen.");
}

bool Application::isDeviceSuitable(
	VkPhysicalDevice device, 
	const QueueFamilyIndices &queueFamilyIndices,
	const SwapChainSupportDetails &swapChainSupportDetails,
	const VkPhysicalDeviceProperties &deviceProperties, 
	const VkPhysicalDeviceFeatures &deviceFeatures
) {
	bool familyIndicesComplete = queueFamilyIndices.isComplete();
	bool extensionsSupported = checkDeviceRequiredExtensionsSupport(device);
	bool swapChainAdequate = false;
	
	if (extensionsSupported) {
		swapChainAdequate = !swapChainSupportDetails.formats.empty() && !swapChainSupportDetails.presentModes.empty();
	}

	return familyIndicesComplete && extensionsSupported && swapChainAdequate;
}

bool Application::checkDeviceRequiredExtensionsSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	for (const auto &requiredExtension : requiredDeviceExtensions) {
		bool found = false;
		for (const auto &availableExtension : availableExtensions) {
			if (!strcmp(requiredExtension, availableExtension.extensionName)) {
				found = true;
				break;
			}
		}
		if (!found) {
			return false;
		}
	}

	return true;
}

Application::SwapChainSupportDetails Application::querySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

VkSurfaceFormatKHR Application::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
{
	if(availableFormats.empty()) {
		throw std::runtime_error("chooseSwapSurfaceFormat: availableFormats must not be empty");
	}

	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkExtent2D Application::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height),
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

VkPresentModeKHR Application::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
{
	for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

Application::QueueFamilyIndices Application::findNeededQueueFamilyIndices(VkPhysicalDevice device)
{
	QueueFamilyIndices indices{};

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	uint32_t i = 0;
	for (const auto& queueFamily : queueFamilies) {
		VkBool32 presentSupport = 0;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
		
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphics = i;
		}
		if (presentSupport) {
			indices.present = i;
		}

		if(indices.isComplete()) {
			break;
		}

		i++;
	}

	return indices;
}

void Application::createLogicalDevice()
{
	log.info("creating logical device...");

	selectedQueueFamilyIndices = findNeededQueueFamilyIndices(physicalDevice);
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueIndices{selectedQueueFamilyIndices.graphics.value(), selectedQueueFamilyIndices.present.value()};

	float priority = 1.f;
	for (const auto &index : uniqueIndices) {
		VkDeviceQueueCreateInfo queueCreateInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = index,
			.queueCount = 1,
			.pQueuePriorities = &priority
		};
		queueCreateInfos.push_back(queueCreateInfo);
	}


	VkPhysicalDeviceFeatures deviceFeatures{};

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = queueCreateInfos.size();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();
	createInfo.enabledExtensionCount = requiredDeviceExtensions.size();

	// there is no longer a distinction between device and instance specific layers, but setting those fields for backwards compatibility
	if (isValidationLayersEnabled) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(requiredValidationLayers.size());
		createInfo.ppEnabledLayerNames = requiredValidationLayers.data();
	}

	VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
	if (result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("vkCreateDevice failed with code {}", (int32_t) result));
	}

	vkGetDeviceQueue(device, selectedQueueFamilyIndices.graphics.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(device, selectedQueueFamilyIndices.present.value(), 0, &presentQueue);
}

void Application::createSwapChain()
{
	log.info("creating swap chain...");

	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	uint32_t imageCount = std::clamp(
		swapChainSupport.capabilities.minImageCount + 1, 
		swapChainSupport.capabilities.minImageCount, 
		swapChainSupport.capabilities.maxImageCount > 0 ? swapChainSupport.capabilities.maxImageCount : std::numeric_limits<uint32_t>::max()
	);

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	// if the present and graphics queues are seperate, we must explicitly share their images
	uint32_t queueFamilyIndices[] = {selectedQueueFamilyIndices.graphics.value(), selectedQueueFamilyIndices.present.value()};
	if(selectedQueueFamilyIndices.graphics != selectedQueueFamilyIndices.present) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	VkResult result = vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain);
	if (result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("vkCreateSwapchainKHR failed with code {}", (int32_t) result));
	}

	// get and store the handles to the swap chain images (there may be more than requested)
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;

	log.info(fmt::format("swap chain with {} images created.", imageCount));
}

void Application::createImageViews()
{
	log.info("creating swap chain image views...");

	swapChainImageViews.resize(swapChainImages.size());

	for (size_t i = 0; i < swapChainImageViews.size(); ++i) {
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapChainImageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		VkResult result = vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]);
		if (result != VK_SUCCESS) {
			throw std::runtime_error(fmt::format("vkCreateImageView failed with code {}", (int32_t) result));
		}
	}
}

void Application::createRenderPass()
{
	log.info("creating render pass...");

	VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0; // layout (location = 0)
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	VkResult result = vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass);
	if (result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("vkCreateRenderPass failed with code {}", (int32_t)  result));
	}
}

void Application::createGraphicsPipeline()
{
	log.info("creating graphics pipeline...");

	// shaders
	auto vertexShader = readFile("compiled/shader.vert");
	auto fragmentShader = readFile("compiled/shader.frag");

	VkShaderModule vertShaderModule = createShaderModule(vertexShader);
    VkShaderModule fragShaderModule = createShaderModule(fragmentShader);

	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStageInfos = {};
	shaderStageInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStageInfos[0].module = vertShaderModule;
	shaderStageInfos[0].pName = "main";

	shaderStageInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStageInfos[1].module = fragShaderModule;
	shaderStageInfos[1].pName = "main";


	// fixed function setup
	std::array<VkDynamicState, 2> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.f;
	rasterizer.depthBiasClamp = 0.f;
	rasterizer.depthBiasSlopeFactor = 0.f;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.f;
	colorBlending.blendConstants[1] = 0.f;
	colorBlending.blendConstants[2] = 0.f;
	colorBlending.blendConstants[3] = 0.f;

	// pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
	if (result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("vkCreatePipelineLayout failed with code {}", (int32_t) result));
	}

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = shaderStageInfos.size();
	pipelineInfo.pStages = shaderStageInfos.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline);
	if (result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("vkCreateGraphicsPipelines", (int32_t) result));
	}

	vkDestroyShaderModule(device, vertShaderModule, nullptr);
	vkDestroyShaderModule(device, fragShaderModule, nullptr);
}

void Application::createFramebuffers()
{
	swapChainFramebuffers.resize(swapChainImageViews.size());

	for (size_t i = 0; i < swapChainImageViews.size(); i++) {
		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = &swapChainImageViews[i];
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		VkResult result = vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]);
		if (result != VK_SUCCESS) {
			throw std::runtime_error(fmt::format("vkCreateFramebuffer failed with code {}", (int32_t) result));
		}
	}
}

void Application::createCommandPool()
{
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = selectedQueueFamilyIndices.graphics.value();

	VkResult result = vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
	if (result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("vkCreateCommandPool with code {}", (int32_t) result));
	}
}

void Application::createCommandBuffer()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	VkResult result = vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
	if (result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("vkAllocateCommandBuffers failed with code {}", (int32_t) result));
	}
}

void Application::createSyncObjects()
{
	VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkResult result = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore);
	if (result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("vkCreateSemaphore failed with code {}", (int32_t) result));
	}
	result = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore);
	if (result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("vkCreateSemaphore failed with code {}", (int32_t) result));
	}
	result = vkCreateFence(device, &fenceInfo, nullptr, &frameFence);
	if (result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("vkCreateFence failed with code {}", (int32_t) result));
	}
}

void Application::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	if (imageIndex >= swapChainFramebuffers.size()) {
		throw std::invalid_argument("imageIndex is out of bounds");
	}

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;

	VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
	if (result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("vkBeginCommandBuffer failed with code {}", (int32_t) result));
	}

	// begin render pass
	VkClearValue clearColor = {{{0.f, 0.f, 0.f, 1.f}}};
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = swapChainExtent;
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

	VkViewport viewport{};
	viewport.x = 0.f;
	viewport.y = 0.f;
	viewport.width = static_cast<float>(swapChainExtent.width);
	viewport.height = static_cast<float>(swapChainExtent.height);
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = swapChainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	vkCmdDraw(commandBuffer, 3, 1, 0, 0);
	vkCmdEndRenderPass(commandBuffer);

	result = vkEndCommandBuffer(commandBuffer);
	if (result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("vkEndCommandBuffer failed with code {}", (int32_t) result));
	}
}

VkShaderModule Application::createShaderModule(const std::vector<std::byte> &shader)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = shader.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(shader.data());

	VkShaderModule shaderModule = VK_NULL_HANDLE;
	VkResult result = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
	if (result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("vkCreateShaderModule failed with code {}", (int32_t) result));
	}

	return shaderModule;
}

void Application::draw()
{
	vkWaitForFences(device, 1, &frameFence, VK_TRUE, UINT64_MAX);
	vkResetFences(device, 1, &frameFence);

	uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
		device, 
		swapChain, 
		UINT64_MAX, 
		imageAvailableSemaphore, 
		VK_NULL_HANDLE, 
		&imageIndex
	);
	if (result != VK_SUCCESS) {
		log.error("vkAcquireNextImageKHR returned code {}", (int32_t) result);
	}

	result = vkResetCommandBuffer(commandBuffer, 0);
	if (result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("vkResetCommandBuffer failed with code {}", (int32_t) result));
	}
	recordCommandBuffer(commandBuffer, imageIndex);

	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &renderFinishedSemaphore;

	result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, frameFence);
	if (result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("vkQueueSubmit failed with code {}", (int32_t) result));
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapChain;
	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(presentQueue, &presentInfo);
	if (result != VK_SUCCESS) {
		log.error("vkQueuePresentKHR returned code {}", (int32_t) result);
	}
}

void Application::mainLoop()
{
	log.info("starting main loop...");
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		draw();
	}

	vkDeviceWaitIdle(device);
}

void Application::cleanup()
{
	log.info("cleaning up...");

    vkDestroyFence(device, frameFence, nullptr);
    vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
	vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
	vkDestroyCommandPool(device, commandPool, nullptr);
	for (auto &framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyRenderPass(device, renderPass, nullptr);
	for (auto &imageView : swapChainImageViews) {
		vkDestroyImageView(device, imageView, nullptr);
	}
	vkDestroySwapchainKHR(device, swapChain, nullptr);
	vkDestroyDevice(device, nullptr);

	if (isValidationLayersEnabled) {
		auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (vkDestroyDebugUtilsMessengerEXT == nullptr) {
			throw std::runtime_error("cleanup failed: address of vkDestroyDebugUtilsMessengerEXT could not be retrieved");
		}
		vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	}

	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);
	glfwDestroyWindow(window);
	glfwTerminate();
}

std::vector<std::byte> Application::readFile(std::filesystem::path path)
{
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	auto len = file.tellg();
	if (len <= -1) {
		throw std::runtime_error(fmt::format("tellg on file {} failed", path.string()));
	}
	if (len == 0) {
		return std::vector<std::byte>{};
	}
	std::vector<std::byte> buf(len);
	file.seekg(0).read(reinterpret_cast<char *>(buf.data()), len);

	if(file.fail()) {
		throw std::runtime_error(fmt::format("reading file {} failed", path.string()));
	}

	file.close();

	return buf;
}


VKAPI_ATTR VkBool32 VKAPI_CALL Application::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
) {
	auto *myThis = static_cast<Application *>(pUserData);

    myThis->log.info("validation layer: {}", pCallbackData->pMessage);

    return VK_FALSE;
}
