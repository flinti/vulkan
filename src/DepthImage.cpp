#include "DepthImage.h"
#include"VkHelpers.h"

#include <vulkan/vulkan_core.h>

DepthImage::DepthImage(DeviceAllocator &allocator, VkDevice device, uint32_t width, uint32_t height)
    : allocator(allocator),
    device(device),
    depthImage(VK_NULL_HANDLE, VK_NULL_HANDLE)
{
    createImage(width, height);
    createImageView();
}

DepthImage::~DepthImage()
{
    if (depthImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, depthImageView, nullptr);
    }
    if (depthImage.first != VK_NULL_HANDLE && depthImage.second != VK_NULL_HANDLE) {
        allocator.free(depthImage);
    }
}

VkImage DepthImage::getImageHandle() const
{
    return depthImage.first;
}

VkImageView DepthImage::getImageViewHandle() const
{
    return depthImageView;
}

void DepthImage::createImage(uint32_t width, uint32_t height)
{
    depthImage = allocator.allocateImageAttachment(
        width, 
        height, 
        VK_FORMAT_D24_UNORM_S8_UINT, 
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

void DepthImage::createImageView()
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = depthImage.first;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_D24_UNORM_S8_UINT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VK_ASSERT(vkCreateImageView(device, &viewInfo, nullptr, &depthImageView));
}
