#ifndef DEVICE_H_
#define DEVICE_H_

#include "SwapChain.h"

#include <spdlog/logger.h>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>
#include <optional>

class Instance;

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;

    bool isComplete() const
    {
        return graphics.has_value() && present.has_value();
    }
};

class Device
{
public:
    Device(
        Instance &instance, 
        VkSurfaceKHR surface, 
        std::vector<const char *> extensionsToEnable = {}
    );
    ~Device();

    VkInstance getInstanceHandle();
    VkPhysicalDevice getPhysicalDeviceHandle();
    VkDevice getDeviceHandle();
    VkQueue getGraphicsQueue();
    VkQueue getPresentQueue();
    const QueueFamilyIndices &getQueueFamilyIndices() const;

    void waitDeviceIdle();
private:
    void createInstance();
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

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    QueueFamilyIndices selectedQueueFamilyIndices{};
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    std::vector<const char *> extensionsToEnable;

    VkSurfaceKHR surface;
    Instance &instance;
};

#endif
