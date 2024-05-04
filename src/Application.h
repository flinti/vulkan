#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cstdint>
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
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

	Application(spdlog::logger &log, bool enableValidationLayers);
	~Application();
	void run();
private:
	spdlog::logger &log;

    GLFWwindow *window = nullptr;
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    QueueFamilyIndices selectedQueueFamilyIndices;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    VkFormat swapChainImageFormat{};
    VkExtent2D swapChainExtent{};
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
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
    QueueFamilyIndices findNeededQueueFamilyIndices(VkPhysicalDevice device);
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void mainLoop();
    void cleanup();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT, 
        VkDebugUtilsMessageTypeFlagsEXT, 
        const VkDebugUtilsMessengerCallbackDataEXT*, 
        void *
    );
};






#endif
