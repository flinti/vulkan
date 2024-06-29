#ifndef IMAGE_H_
#define IMAGE_H_

#include "DeviceAllocator.h"

#include <utility>
#include <vulkan/vulkan_core.h>
#include <filesystem>

class DeviceAllocator;

class Image
{
public:
    Image(DeviceAllocator &allocator, VkDevice device, const std::filesystem::path &image);
    Image(const Image &) = delete;
    Image(Image &&);
    ~Image();

    VkImage getImageHandle() const;
private:
    std::pair<VkImage, VmaAllocation> createImage(const std::filesystem::path &image);

    DeviceAllocator &allocator;
    VkDevice device;

    std::pair<VkImage, VmaAllocation> image = std::make_pair<VkImage, VmaAllocation>(VK_NULL_HANDLE, VK_NULL_HANDLE);
};

#endif
