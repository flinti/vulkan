#ifndef DEVICEALLOCATOR_H_
#define DEVICEALLOCATOR_H_

#include <utility>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

class Device;

class DeviceAllocator
{
public:
    DeviceAllocator(
        VkInstance instance, 
        VkPhysicalDevice physicalDevice, 
        VkDevice device,
        VkCommandPool immediateTransferPool,
        VkQueue immediateTransferQueue
    );
    ~DeviceAllocator();

    std::pair<VkBuffer, VmaAllocation> allocateDeviceLocalBufferAndTransfer(
        void *data,
        size_t size, 
        VkBufferUsageFlags usage
    );
    std::pair<VkImage, VmaAllocation> allocateImageAttachment(
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageUsageFlags usage
    );
    void free(std::pair<VkBuffer, VmaAllocation> allocation);
    void free(std::pair<VkImage, VmaAllocation> allocation);
private:
    std::pair<VkBuffer, VmaAllocation> allocateBuffer(
        size_t size, 
        VkBufferUsageFlags usage, 
        VkMemoryPropertyFlags properties,
        VmaAllocationCreateFlags allocFlags = 0
    );
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    VmaAllocator allocator = VK_NULL_HANDLE;

    VkCommandPool immediateTransferPool;
    VkQueue immediateTransferQueue;
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
};

#endif
