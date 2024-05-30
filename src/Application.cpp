#include "Application.h"
#include "GraphicsPipeline.h"
#include "RenderPass.h"
#include "VkHelpers.h"

#include <GLFW/glfw3.h>
#include <chrono>
#include <fmt/core.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <thread>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <sstream>


Application::Application(spdlog::logger &log, bool enableValidationLayers, uint32_t concurrentFrames)
	: log(log),
	concurrentFrames(concurrentFrames),
	testingMesh(Mesh::createRegularPolygon(1.f, 6))
{
	initWindow();
	initVulkan(enableValidationLayers);
}

Application::~Application()
{
	log.info("running application destructor...");
	cleanup();
}


void Application::run()
{
	startedAtTimePoint = std::chrono::high_resolution_clock::now();
	mainLoop();
}


void Application::setTargetFps(float targetFps)
{
	this->targetFps = targetFps;
}

void Application::initWindow()
{
	log.info("initializing window...");
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Demo", nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResized);
}

void Application::initVulkan(bool validationLayers)
{
	log.info("initializing vulkan...");

	// request the required extensions
	uint32_t glfwRequiredExtensionsCount = 0;
	const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwRequiredExtensionsCount);
	if (glfwExtensions == nullptr) {
		throw std::runtime_error("glfwGetRequiredInstanceExtensions returned NULL");
	}
	std::vector<const char *> instanceExtensions(glfwExtensions, glfwExtensions + glfwRequiredExtensionsCount);

	instance = std::make_unique<Instance>(log, instanceExtensions, validationLayers);
	
	VK_ASSERT(glfwCreateWindowSurface(instance->getHandle(), window, nullptr, &surface));
	
	device = std::make_unique<Device>(
		log, 
		*instance, 
		surface,
		std::vector<const char *>{ 
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		}
	);
	createRenderPassAndSwapChain();
	graphicsPipeline = std::make_unique<GraphicsPipeline>(device->getDeviceHandle(), *renderPass);
	createCommandPools();
	vertexBuffer = std::make_unique<Buffer>(
		(void *) testingMesh.getVertexData().data(), 
		testingMesh.getVertexDataSize(), 
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		device->getPhysicalDeviceHandle(), 
		device->getDeviceHandle(), 
		transferCommandPool, 
		device->getGraphicsQueue()
	);
	indexBuffer = std::make_unique<Buffer>(
		(void *) testingMesh.getIndexData().data(), 
		testingMesh.getIndexDataSize(), 
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		device->getPhysicalDeviceHandle(), 
		device->getDeviceHandle(), 
		transferCommandPool, 
		device->getGraphicsQueue()
	);
	
	frames.reserve(concurrentFrames);
	for (size_t i = 0; i < concurrentFrames; ++i) {
		frames.emplace_back(std::make_unique<Frame>(
			device->getDeviceHandle(), 
			device->getQueueFamilyIndices().graphics.value()
		));
	}
}

VkSurfaceFormatKHR Application::chooseSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
{
	if(availableFormats.empty()) {
		throw std::runtime_error("chooseSwapSurfaceFormat: availableFormats must not be empty");
	}

	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB 
			&& availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
		) {
			return availableFormat;
		}
	}

	return availableFormats[0];
}

void Application::createRenderPassAndSwapChain()
{
	log.info("creating swap chain...");

	auto swapChainSupportDetails = SwapChain::querySwapChainSupportDetails(
		device->getPhysicalDeviceHandle(), surface
	);
	auto chosenSurfaceFormat = chooseSwapChainSurfaceFormat(
		swapChainSupportDetails.formats
	);

	renderPass = std::make_unique<RenderPass>(
		device->getDeviceHandle(), 
		chosenSurfaceFormat.format
	);
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
		*device,
		surface,
		*renderPass,
		static_cast<uint32_t>(wdt), 
		static_cast<uint32_t>(hgt)
	);
}

void Application::recreateSwapChain() 
{
	device->waitDeviceIdle();

	SwapChainSupportDetails swapChainSupportDetails = SwapChain::querySwapChainSupportDetails(
		device->getPhysicalDeviceHandle(), 
		surface
	);
	VkSurfaceFormatKHR chosenSurfaceFormat = swapChain->getSurfaceFormat();
	swapChain.reset();
    createSwapChain(swapChainSupportDetails, chosenSurfaceFormat);
}

void Application::createCommandPools()
{
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	poolInfo.queueFamilyIndex = device->getQueueFamilyIndices().graphics.value();

	VK_ASSERT(vkCreateCommandPool(
		device->getDeviceHandle(), 
		&poolInfo, 
		nullptr, 
		&transferCommandPool
	));
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
	float vpWdt = static_cast<float>(swapChainExtent.width);
	float vpHgt = static_cast<float>(swapChainExtent.height);

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

	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>( currentTime - startTime).count();

	glm::mat4 m = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 v = glm::lookAt(glm::vec3(0.0f, -4.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 p = glm::perspective(glm::radians(45.0f), vpWdt / vpHgt, 0.1f, 100.0f);
	glm::mat4 mvp = p * v * m;
	mvp[1][1] *= -1.f;
	graphicsPipeline->pushConstants(commandBuffer, static_cast<void *>(&mvp), sizeof(mvp));

	VkViewport viewport{};
	viewport.x = 0.f;
	viewport.y = 0.f;
	viewport.width = vpWdt;
	viewport.height = vpHgt;
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
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getHandle(), 0, VK_INDEX_TYPE_UINT16);

	vkCmdDrawIndexed(
		commandBuffer, 
		testingMesh.getIndexData().size(), 
		1, 
		0, 
		0, 
		0
	);

	vkCmdEndRenderPass(commandBuffer);

	result = vkEndCommandBuffer(commandBuffer);
	if (result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("vkEndCommandBuffer failed with code {}", (int32_t) result));
	}
}

void Application::draw()
{
	VkFence fence = frames[currentFrameIndex]->getFence();
	VkSemaphore imageAvailableSemaphore = frames[currentFrameIndex]->getImageAvailableSemaphore();
	VkSemaphore renderFinishedSemaphore = frames[currentFrameIndex]->getRenderFinishedSemaphore();
	VkCommandBuffer commandBuffer = frames[currentFrameIndex]->getCommandBuffer();
	VkQueue graphicsQueue = device->getGraphicsQueue();
    VkQueue presentQueue = device->getPresentQueue();

	vkWaitForFences(
		device->getDeviceHandle(), 
		1, 
		&fence, 
		VK_TRUE, 
		UINT64_MAX
	);

	uint32_t imageIndex;
    VkResult result = swapChain->acquireNextImage(
		&imageIndex, 
		imageAvailableSemaphore
	);
	// framebufferJustResized is handled near the end of this function (deviating from the tutorial) 
	// to avoid that the image available semaphore occasionally remains signalled
	if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS) {
		log.error("vkAcquireNextImageKHR failed with code {}", (int32_t) result);
	}

	vkResetFences(device->getDeviceHandle(), 1, &fence);

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

	result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, fence);
	if (result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("vkQueueSubmit failed with code {}", (int32_t) result));
	}

	result = swapChain->queuePresent(presentQueue, imageIndex, renderFinishedSemaphore);
	if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR || needsSwapChainRecreation) {
		needsSwapChainRecreation = false;
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		log.error("vkQueuePresentKHR failed with code {}", (int32_t) result);
	}

	currentFrameIndex = (currentFrameIndex + 1) % concurrentFrames;
}

void Application::mainLoop()
{
	log.info("starting main loop...");

	auto prevFrameTime = std::chrono::high_resolution_clock::now();

	while (!glfwWindowShouldClose(window)) {
		auto beginFrameTime = std::chrono::high_resolution_clock::now();
		glfwPollEvents();

		++frameCounter;
		secondsRunning = std::chrono::duration<float, std::chrono::seconds::period>(
			std::chrono::high_resolution_clock::now() - startedAtTimePoint
		).count();

		if (!paused) {
			draw();
		}

		auto endFrameTime = std::chrono::high_resolution_clock::now();
		
		// limit frame rate
		float frameDuration = std::chrono::duration<float, std::chrono::seconds::period>(
			endFrameTime - beginFrameTime
		).count();

		float targetFrameDuration = 1.f / targetFps;
		float sleepSec = targetFrameDuration - frameDuration;
		if (sleepSec > 0.f) {
			std::this_thread::sleep_for(
				std::chrono::duration<float, std::chrono::seconds::period>(sleepSec)
			);
		}

		// measure frame rate
		frameDuration = std::chrono::duration<float, std::chrono::seconds::period>(
			endFrameTime - prevFrameTime
		).count();

		prevFrameTime = endFrameTime;
		frameRate = 1.f / frameDuration;
		updateInfoDisplay();
	}

	device->waitDeviceIdle();
}

void Application::updateInfoDisplay()
{
	std::stringstream s;
	s << "Demo " << std::setprecision(3) << frameRate << " fps";
	glfwSetWindowTitle(window, s.str().c_str());
}

void Application::cleanup()
{
	log.info("cleaning up...");

	swapChain.reset();
	indexBuffer.reset();
	vertexBuffer.reset();
	vkDestroyCommandPool(device->getDeviceHandle(), transferCommandPool, nullptr);

	for (auto &&frame : frames) {
		frame.reset();
	}

	graphicsPipeline.reset();
	renderPass.reset();
	device.reset();
	vkDestroySurfaceKHR(instance->getHandle(), surface, nullptr);
	instance.reset();
	glfwDestroyWindow(window);
	glfwTerminate();
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
