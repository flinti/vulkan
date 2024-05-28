#include "Buffer.h"

#include <cstdint>
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>
#include <stdexcept>

Buffer::Buffer(void *data, size_t size, VkPhysicalDevice physicalDevice, VkDevice device, VkCommandPool transferPool, VkQueue transferQueue)
    : physicalDevice(physicalDevice), 
    device(device), 
    transferPool(transferPool), 
    transferQueue(transferQueue)
{
    createAndFillBuffer(data, size);
}

Buffer::~Buffer()
{
    vkDestroyBuffer(device, buffer, nullptr);
    vkFreeMemory(device, bufferMemory, nullptr);
}

VkBuffer Buffer::getHandle() const
{
    return buffer;
}

uint32_t Buffer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("Buffer::findMemoryType: failed to find suitable memory type!");
}

void Buffer::createBuffer(VkBuffer &buffer, VkDeviceMemory &deviceMemory, size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);
    if (result != VK_SUCCESS) {
        throw std::runtime_error(fmt::format("vkCreateBuffer failed with code {}", (int32_t) result));
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    result = vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory);
    if (result != VK_SUCCESS) {
        throw std::runtime_error(fmt::format("vkAllocateMemory failed with code {}", (int32_t) result));
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void Buffer::createAndFillBuffer(void *data, size_t size)
{
    createBuffer(buffer, bufferMemory, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void *bufData;
    vkMapMemory(device, bufferMemory, 0, size, 0, &bufData);
    std::memcpy(bufData, data, size);
    vkUnmapMemory(device, bufferMemory);
}

