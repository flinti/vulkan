#include "Application.h"
#include <GLFW/glfw3.h>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <set>
#include <sstream>


Application::Application(spdlog::logger &log, bool enableValidationLayers)
	: log(log),
	isValidationLayersEnabled(enableValidationLayers)
{
}
Application::~Application()
{
	log.info("running application destructor...");
}


void Application::run()
{
	initWindow();
	initVulkan();
	mainLoop();
	cleanup();
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

	for (const char *layer : validationLayers) {
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
		
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
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
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		bool suitable = isDeviceSuitable(device, queueFamilyIndices, deviceProperties, deviceFeatures);
		if (suitable) {
			physicalDevice = device;
		}

		logLine << "\n\tID " << deviceProperties.deviceID << ": " << deviceProperties.deviceName;
	}
	log.info(logLine.str());

	if (physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("GPUs were found, but no device is suitable!");
	}

	log.info("device chosen.");
}

bool Application::isDeviceSuitable(
	VkPhysicalDevice device, 
	const QueueFamilyIndices &queueFamilyIndices, 
	const VkPhysicalDeviceProperties &deviceProperties, 
	const VkPhysicalDeviceFeatures &deviceFeatures
) {
	return queueFamilyIndices.isComplete();
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
	log.info("Creating logical device...");

	QueueFamilyIndices queueFamilyIndices = findNeededQueueFamilyIndices(physicalDevice);
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueIndices{queueFamilyIndices.graphics.value(), queueFamilyIndices.present.value()};

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

	// there is no longer a distinction between device and instance specific layers, but setting those fields for backwards compatibility
	if (isValidationLayersEnabled) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}

	VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
	if (result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("vkCreateDevice failed with code {}", (int32_t) result));
	}

	vkGetDeviceQueue(device, queueFamilyIndices.graphics.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(device, queueFamilyIndices.present.value(), 0, &presentQueue);
}

void Application::mainLoop()
{
	log.info("starting main loop...");
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}
}

void Application::cleanup()
{
	log.info("cleaning up...");

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
