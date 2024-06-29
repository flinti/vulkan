#include "Material.h"
#include "DeviceAllocator.h"
#include "Image.h"
#include "VkHelpers.h"
#include <vulkan/vulkan_core.h>

Material::Material(
    DeviceAllocator &allocator, 
    VkDevice device, 
    const std::filesystem::path &imageFile, 
    std::string name
)
    : allocator(allocator),
    device(device),
    image(allocator, device, imageFile),
    name(name)
{
    imageView = createImageView();
    sampler = createSampler();
}

Material::Material(Material &&other)
    : allocator(other.allocator),
    device(other.device),
    image(std::move(other.image)),
    imageView(other.imageView),
    sampler(other.sampler),
    name(std::move(other.name))
{
    other.imageView = VK_NULL_HANDLE;
    other.sampler = VK_NULL_HANDLE;
}

Material::~Material()
{
    if (imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    if (sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, sampler, nullptr);
    }
}

VkImage Material::getImageHandle() const
{
    return image.getImageHandle();
}

VkImageView Material::getImageViewHandle() const
{
    return imageView;
}

VkSampler Material::getSamplerHandle() const
{
    return sampler;
}

VkImageView Material::createImageView()
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image.getImageHandle();
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    VK_ASSERT(vkCreateImageView(device, &viewInfo, nullptr, &imageView));
    return imageView;
}

VkSampler Material::createSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    VkSampler sampler;
    VK_ASSERT(vkCreateSampler(device, &samplerInfo, nullptr, &sampler));
    return sampler;
}

