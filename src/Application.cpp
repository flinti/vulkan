#include "Application.h"
#include "GraphicsPipeline.h"
#include "RenderPass.h"

#include <GLFW/glfw3.h>
#include <fmt/core.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <set>
#include <sstream>


Application::Application(spdlog::logger &log, bool enableValidationLayers, uint32_t concurrentFrames)
	: log(log),
	isValidationLayersEnabled(enableValidationLayers),
	concurrentFrames(concurrentFrames)
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
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResized);
}

void Application::initVulkan()
{
	log.info("initializing vulkan...");
	createInstance();
	setupDebugMessenger();
	createSurface();
	findAndChooseDevice();
	createLogicalDevice();
	createRenderPassAndSwapChain();
	graphicsPipeline = std::make_unique<GraphicsPipeline>(device, *renderPass);
	createCommandPools();
	vertexBuffer = std::make_unique<Buffer>(
		(void *) vertices.data(), 
		vertices.size()*sizeof(vertices[0]), 
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		physicalDevice, device, 
		transferCommandPool, 
		graphicsQueue
		);
	createCommandBuffers();
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
		SwapChainSupportDetails swapChainSupportDetails = SwapChain::querySwapChainSupportDetails(device, surface);
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


VkSurfaceFormatKHR Application::chooseSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
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

void Application::createRenderPassAndSwapChain()
{
	log.info("creating swap chain...");

	auto swapChainSupportDetails = SwapChain::querySwapChainSupportDetails(physicalDevice, surface);
	auto chosenSurfaceFormat = chooseSwapChainSurfaceFormat(swapChainSupportDetails.formats);

	renderPass = std::make_unique<RenderPass>(device, chosenSurfaceFormat.format);
	createSwapChain(swapChainSupportDetails, chosenSurfaceFormat);
}

void Application::createSwapChain(const SwapChainSupportDetails &swapChainSupportDetails, const VkSurfaceFormatKHR &chosenSurfaceFormat)
{
	int wdt = 0;
	int hgt = 0;
	glfwGetFramebufferSize(window, &wdt, &hgt);
	
	swapChain = std::make_unique<SwapChain>(
		swapChainSupportDetails,
		chosenSurfaceFormat,
		device,
		surface,
		*renderPass,
		static_cast<uint32_t>(wdt), 
		static_cast<uint32_t>(hgt), 
		selectedQueueFamilyIndices.graphics.value(),
		selectedQueueFamilyIndices.present.value()
	);
}

void Application::recreateSwapChain() 
{
    vkDeviceWaitIdle(device);

	SwapChainSupportDetails swapChainSupportDetails = swapChain->getSwapChainSupportDetails();
	VkSurfaceFormatKHR chosenSurfaceFormat = swapChain->getSurfaceFormat();
	swapChain.reset();
    createSwapChain(swapChainSupportDetails, chosenSurfaceFormat);
}

void Application::createCommandPools()
{
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = selectedQueueFamilyIndices.graphics.value();

	VkResult result = vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
	if (result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("vkCreateCommandPool failed with code {}", (int32_t) result));
	}

	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	poolInfo.queueFamilyIndex = selectedQueueFamilyIndices.graphics.value();

	result = vkCreateCommandPool(device, &poolInfo, nullptr, &transferCommandPool);
	if (result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("vkCreateCommandPool failed with code {}", (int32_t) result));
	}
}

void Application::createCommandBuffers()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = concurrentFrames;

	commandBuffers.assign(concurrentFrames, VK_NULL_HANDLE);

	VkResult result = vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data());
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

	imageAvailableSemaphores.assign(concurrentFrames, VK_NULL_HANDLE);
	renderFinishedSemaphores.assign(concurrentFrames, VK_NULL_HANDLE);
	frameFences.assign(concurrentFrames, VK_NULL_HANDLE);

	for (size_t i = 0; i < concurrentFrames; ++i) {
		VkResult result = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]);
		if (result != VK_SUCCESS) {
			throw std::runtime_error(fmt::format("vkCreateSemaphore failed with code {}", (int32_t) result));
		}
		result = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]);
		if (result != VK_SUCCESS) {
			throw std::runtime_error(fmt::format("vkCreateSemaphore failed with code {}", (int32_t) result));
		}
		result = vkCreateFence(device, &fenceInfo, nullptr, &frameFences[i]);
		if (result != VK_SUCCESS) {
			throw std::runtime_error(fmt::format("vkCreateFence failed with code {}", (int32_t) result));
		}
	}
}

void Application::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	if (imageIndex >= swapChain->getImageCount()) {
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

	VkExtent2D swapChainExtent = swapChain->getExtent();

	// begin render pass
	VkClearValue clearColor = {{{0.f, 0.f, 0.f, 1.f}}};
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass->getHandle();
	renderPassInfo.framebuffer = swapChain->getFramebuffer(imageIndex);
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = swapChainExtent;
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	graphicsPipeline->bind(commandBuffer);

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


	VkBuffer vertexBuffers[] = {vertexBuffer->getHandle()};
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

	vkCmdDraw(commandBuffer, vertices.size(), 1, 0, 0);
	vkCmdEndRenderPass(commandBuffer);

	result = vkEndCommandBuffer(commandBuffer);
	if (result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("vkEndCommandBuffer failed with code {}", (int32_t) result));
	}
}

void Application::draw()
{
	vkWaitForFences(device, 1, &frameFences[currentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
    VkResult result = swapChain->acquireNextImage(&imageIndex, imageAvailableSemaphores[currentFrame]);
	// framebufferJustResized is handled near the end of this function (deviating from the tutorial) 
	// to avoid that the image available semaphore occasionally remains signalled
	if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS) {
		log.error("vkAcquireNextImageKHR failed with code {}", (int32_t) result);
	}

	vkResetFences(device, 1, &frameFences[currentFrame]);

	result = vkResetCommandBuffer(commandBuffers[currentFrame], 0);
	if (result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("vkResetCommandBuffer failed with code {}", (int32_t) result));
	}
	recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &imageAvailableSemaphores[currentFrame];
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &renderFinishedSemaphores[currentFrame];

	result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, frameFences[currentFrame]);
	if (result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("vkQueueSubmit failed with code {}", (int32_t) result));
	}

	result = swapChain->queuePresent(presentQueue, imageIndex, renderFinishedSemaphores[currentFrame]);
	if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR || needsSwapChainRecreation) {
		needsSwapChainRecreation = false;
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		log.error("vkQueuePresentKHR failed with code {}", (int32_t) result);
	}

	currentFrame = (currentFrame + 1) % concurrentFrames;
}

void Application::mainLoop()
{
	log.info("starting main loop...");
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		if (!paused) {
			draw();
		}
	}

	vkDeviceWaitIdle(device);
}

void Application::cleanup()
{
	log.info("cleaning up...");

	for (size_t i = 0; i < concurrentFrames; ++i) {
		vkDestroyFence(device, frameFences[i], nullptr);
		vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
	}

	swapChain.reset();
	vertexBuffer.reset();
	vkDestroyCommandPool(device, transferCommandPool, nullptr);
	vkDestroyCommandPool(device, commandPool, nullptr);
	graphicsPipeline.reset();
	renderPass.reset();
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

void Application::framebufferResized(GLFWwindow* window, int width, int height)
{
	auto *myThis = static_cast<Application *>(glfwGetWindowUserPointer(window));
	if (width <= 0 || height <= 0) {
		myThis->paused = true;
	}
	else if (myThis->paused == true) {
		myThis->paused = false;
	}
	myThis->needsSwapChainRecreation = true;
}
