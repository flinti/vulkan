#ifndef FRAME_H_
#define FRAME_H_

#include <vulkan/vulkan_core.h>

class Frame
{
public:
    Frame(VkDevice device, uint32_t renderQueueFamilyIndex);
    ~Frame();

    const VkCommandBuffer &getCommandBuffer() const;
    const VkFence &getFence() const;
    const VkSemaphore &getImageAvailableSemaphore() const;
    const VkSemaphore &getRenderFinishedSemaphore() const;
private:
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;

    VkDevice device;
};

#endif
