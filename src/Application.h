#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include "GraphicsPipeline.h"
#include "Vertex.h"
#include "RenderPass.h"
#include "SwapChain.h"

#include <cstddef>
#include <memory>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <spdlog/spdlog.h>
#include <vector>
#include <vulkan/vulkan_core.h>


class Application
{
public:
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphics;
        std::optional<uint32_t> present;

        bool isComplete() const
        {
            return graphics.has_value() && present.has_value();
        }
    };

    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

	Application(spdlog::logger &log, bool enableValidationLayers, uint32_t concurrentFrames);
	~Application();
	void run();
private:
	spdlog::logger &log;

    uint32_t concurrentFrames;
    GLFWwindow *window = nullptr;
    bool paused = false;
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    QueueFamilyIndices selectedQueueFamilyIndices;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    std::unique_ptr<SwapChain> swapChain;
    std::unique_ptr<RenderPass> renderPass;
    std::unique_ptr<GraphicsPipeline> graphicsPipeline;
    uint32_t currentFrame = 0;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> frameFences;
    bool needsSwapChainRecreation = false;

    std::vector<Vertex> vertices;
    std::vector<VkExtensionProperties> extensions;
    bool isValidationLayersEnabled = false;
    std::vector<const char *> requiredValidationLayers = {
        "VK_LAYER_KHRONOS_validation",
    };
    std::vector<const char *> requiredDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

    void initWindow();
    void initVulkan();
    void createInstance();
    void fillDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
    void setupDebugMessenger();
    void createSurface();
    bool checkValidationLayersSupported();
    void findAndChooseDevice();
    bool isDeviceSuitable(
        VkPhysicalDevice device, 
        const QueueFamilyIndices &queueFamilyIndices, 
        const SwapChainSupportDetails &swapChainSupportDetails, 
        const VkPhysicalDeviceProperties &deviceProperties, 
        const VkPhysicalDeviceFeatures &deviceFeatures
    );
    bool checkDeviceRequiredExtensionsSupport(VkPhysicalDevice device);
    QueueFamilyIndices findNeededQueueFamilyIndices(VkPhysicalDevice device);
    void createLogicalDevice();
    VkSurfaceFormatKHR chooseSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    void createRenderPassAndSwapChain();
    void createSwapChain(const SwapChainSupportDetails &swapChainSupportDetails, const VkSurfaceFormatKHR &chosenSurfaceFormat);
    void recreateSwapChain();
    void createCommandPool();
    void createVertexBuffer();
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void createCommandBuffers();
    void createSyncObjects();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void mainLoop();
    void draw();
    void cleanup();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT, 
        VkDebugUtilsMessageTypeFlagsEXT, 
        const VkDebugUtilsMessengerCallbackDataEXT*, 
        void *
    );
    static void framebufferResized(GLFWwindow* window, int width, int height);
};






#endif
