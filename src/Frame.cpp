#include "Frame.h"
#include "VkHelpers.h"

#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <utility>
#include <vulkan/vulkan_core.h>

Frame::Frame(VkDevice device, uint32_t renderQueueFamilyIndex)
    : device(device)
{
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = renderQueueFamilyIndex;

	VK_ASSERT(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool));

    VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	VK_ASSERT(vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer));

	VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VK_ASSERT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore));
    VK_ASSERT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore));
    VK_ASSERT(vkCreateFence(device, &fenceInfo, nullptr, &fence));
}

Frame::Frame(Frame &&other)
    : commandPool(other.commandPool),
    commandBuffer(other.commandBuffer),
    fence(other.fence),
    imageAvailableSemaphore(other.imageAvailableSemaphore),
    renderFinishedSemaphore(other.renderFinishedSemaphore),
    descriptorPools(std::move(other.descriptorPools)),
    descriptorSets(std::move(other.descriptorSets)),
    device(other.device)
{
    other.commandPool = VK_NULL_HANDLE;
    other.commandBuffer = VK_NULL_HANDLE;
    other.fence = VK_NULL_HANDLE;
    other.imageAvailableSemaphore = VK_NULL_HANDLE;
    other.renderFinishedSemaphore = VK_NULL_HANDLE;
}

Frame::~Frame()
{
    for (auto &descriptorSet : descriptorSets) {
        descriptorSet.reset();
    }
    for (auto &descriptorPool : descriptorPools) {
        descriptorPool.reset();
    }
    vkDestroyFence(device, fence, nullptr);
    vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
    vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
    vkDestroyCommandPool(device, commandPool, nullptr);
}

const VkCommandBuffer &Frame::getCommandBuffer() const
{
    return commandBuffer;
}

const VkFence &Frame::getFence() const
{
    return fence;
}

const VkSemaphore &Frame::getImageAvailableSemaphore() const
{
    return imageAvailableSemaphore;
}

const VkSemaphore &Frame::getRenderFinishedSemaphore() const
{
    return renderFinishedSemaphore;
}

DescriptorSet &Frame::requestDescriptorSet(
    const DescriptorSetLayout &layout, 
    std::map<uint32_t, VkDescriptorBufferInfo> bufferBindingInfos, 
    std::map<uint32_t, VkDescriptorImageInfo> imageBindingInfos
) {
    auto &pool = requestDescriptorPool(layout);
    DescriptorSet &set =  *descriptorSets.emplace_back(
        std::make_unique<DescriptorSet>(device, pool, bufferBindingInfos, imageBindingInfos)
    );

    spdlog::debug("Frame::requestDescriptorSet: Created new descriptor set {}", (void *) set.getHandle());

    return set;
}

void Frame::updateDescriptorSets()
{
    for (auto &set : descriptorSets) {
        set->updateAll();
    }
}


DescriptorPool &Frame::requestDescriptorPool(const DescriptorSetLayout &layout)
{
    for (auto &existingPool : descriptorPools) {
        if (&existingPool->getDescriptorSetLayout() == &layout) {
            return *existingPool;
        }
    }

    DescriptorPool &pool =  *descriptorPools.emplace_back(
        std::make_unique<DescriptorPool>(device, layout)
    );

    spdlog::debug("Frame::requestDescriptorPool: Created new descriptor pool");

    return pool;
}

