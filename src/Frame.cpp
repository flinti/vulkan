#include "Frame.h"
#include "VkHelpers.h"

#include <spdlog/fmt/fmt.h>
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

Frame::~Frame()
{
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
