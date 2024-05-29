#include <cstdint>
#include <vulkan/vulkan_core.h>

class Buffer
{
public:
    Buffer(void *data, size_t size, VkBufferUsageFlags usage, VkPhysicalDevice physicalDevice, VkDevice device, VkCommandPool transferPool, VkQueue transferQueue);
    ~Buffer();

    VkBuffer getHandle() const;
private:
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void createBuffer(VkBuffer &buffer, VkDeviceMemory &bufferMemory, size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void createAndFillBuffer(const void *data, size_t size, VkBufferUsageFlags usage);

    VkDeviceMemory bufferMemory = VK_NULL_HANDLE;
    VkBuffer buffer = VK_NULL_HANDLE;

    VkCommandPool transferPool;
    VkQueue transferQueue;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
};
