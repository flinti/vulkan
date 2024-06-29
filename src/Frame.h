#ifndef FRAME_H_
#define FRAME_H_

#include "DescriptorSet.h"
#include "DescriptorPool.h"

#include <memory>
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
    const DescriptorSet &getFirstDescriptorSet() const { return *descriptorSets[0]; }
    DescriptorSet &requestDescriptorSet(
        const DescriptorSetLayout &layout, 
        std::map<uint32_t, VkDescriptorBufferInfo> bufferBindingInfos, 
        std::map<uint32_t, VkDescriptorImageInfo> imageBindingInfos
    );
    void updateDescriptorSets();
private:
    DescriptorPool &requestDescriptorPool(const DescriptorSetLayout &layout);

    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;

    std::vector<std::unique_ptr<DescriptorPool>> descriptorPools;
    std::vector<std::unique_ptr<DescriptorSet>> descriptorSets;

    VkDevice device;
};

#endif
