#ifndef FRAME_H_
#define FRAME_H_

#include "DescriptorSet.h"
#include "DescriptorPool.h"

#include <memory>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>

class Frame
{
public:
    Frame(VkDevice device, uint32_t renderQueueFamilyIndex);
    Frame(const Frame &) = delete;
    Frame(Frame &&);
    ~Frame();

    const VkCommandBuffer &getCommandBuffer() const;
    const VkFence &getFence() const;
    const VkSemaphore &getImageAvailableSemaphore() const;
    const VkSemaphore &getRenderFinishedSemaphore() const;
    DescriptorPool &getDescriptorPool(uint32_t concurrencyIndex, const DescriptorSetLayout &layout);
    DescriptorSet &getDescriptorSet(
        uint32_t concurrencyIndex,
        const DescriptorSetLayout &layout, 
        const std::map<uint32_t, VkDescriptorBufferInfo> &bufferBindingInfos, 
        const std::map<uint32_t, VkDescriptorImageInfo> &imageBindingInfos
    );
    void updateDescriptorSets(uint32_t concurrencyIndex);
private:
    DescriptorPool &requestDescriptorPool(const DescriptorSetLayout &layout);

    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;

    std::unordered_map<size_t, std::unordered_map<size_t, std::unique_ptr<DescriptorPool>>> descriptorPools;
    std::unordered_map<size_t, std::unordered_map<size_t, std::unique_ptr<DescriptorSet>>> descriptorSets;

    VkDevice device;
};

#endif
