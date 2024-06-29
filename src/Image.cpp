#include "Image.h"
#include "VkHelpers.h"
#include "third-party/stb_image.h"

#include <filesystem>
#include <spdlog/fmt/fmt.h>
#include <stdexcept>
#include <utility>
#include <vulkan/vulkan_core.h>

Image::Image(DeviceAllocator &allocator, VkDevice device, const std::filesystem::path &image)
    : allocator(allocator),
    device(device),
    image(createImage(image))
{
}

Image::Image(Image &&i)
    : allocator(i.allocator),
    device(i.device),
    image(i.image)
{
    i.image = std::make_pair<VkImage, VmaAllocation>(VK_NULL_HANDLE, VK_NULL_HANDLE);
}

Image::~Image()
{
    if (image.first != VK_NULL_HANDLE && image.second != VK_NULL_HANDLE) {
        allocator.free(image);
    }
}

VkImage Image::getImageHandle() const
{
    return image.first;
}

std::pair<VkImage, VmaAllocation> Image::createImage(const std::filesystem::path &imagePath)
{
    int wdt;
    int hgt;
    int channels;
    auto *imageData = stbi_load(
        imagePath.c_str(), 
        &wdt, 
        &hgt, 
        &channels, 
        STBI_rgb_alpha
    );

    if (!imageData) {
        throw std::runtime_error(fmt::format("Image::createImage: failed to load image {}", imagePath.c_str()));
    }

    image = allocator.allocateDeviceLocalImageAndTransfer(
        (void *) imageData, 
        wdt, 
        hgt, 
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_USAGE_SAMPLED_BIT
    );
    return image;
}
