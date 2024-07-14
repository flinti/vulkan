#include "Material.h"
#include "Device.h"
#include "DeviceAllocator.h"
#include "GraphicsPipeline.h"
#include "Image.h"
#include "VkHelpers.h"
#include <cstdint>
#include <spdlog/spdlog.h>
#include <utility>
#include <vulkan/vulkan_core.h>

Material::Material(
    Device &device,
    const ShaderResource &vertexShader,
    const ShaderResource &fragmentShader,
    const ImageResource &imageResource,
    uint32_t id,
    std::string name
)
    : id(id),
    device(device),
    vertexShader(vertexShader),
    fragmentShader(fragmentShader),
    image(device, imageResource),
    imageView(createImageView()),
    sampler(requestSampler()),
    descriptorSetLayoutBindings(createDescriptorSetLayoutBindings()),
    descriptorImageInfos(createDescriptorImageInfos()),
    name(name)
{
}

Material::Material(Material &&other)
    : device(other.device),
    vertexShader(other.vertexShader),
    fragmentShader(other.fragmentShader),
    image(std::move(other.image)),
    imageView(other.imageView),
    sampler(other.sampler),
    descriptorSetLayoutBindings(std::move(other.descriptorSetLayoutBindings)),
    name(std::move(other.name))
{
    other.imageView = VK_NULL_HANDLE;
    other.sampler = VK_NULL_HANDLE;
}

Material::~Material()
{
    if (imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device.getDeviceHandle(), imageView, nullptr);
    }
}


uint32_t Material::getId() const
{
    return id;
}

const ShaderResource &Material::getVertexShaderResource() const
{
    return vertexShader;
}

const ShaderResource &Material::getFragmentShaderResource() const
{
    return fragmentShader;
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

const std::vector<VkDescriptorSetLayoutBinding> &Material::getDescriptorSetLayoutBindings() const
{
    return descriptorSetLayoutBindings;
}

const std::map<uint32_t, VkDescriptorImageInfo> &Material::getDescriptorImageInfos() const
{
    return descriptorImageInfos;
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
    VK_ASSERT(vkCreateImageView(device.getDeviceHandle(), &viewInfo, nullptr, &imageView));
    return imageView;
}

VkSampler Material::requestSampler()
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

    return device.getObjectCache().getSampler(samplerInfo);
}

std::vector<VkDescriptorSetLayoutBinding> Material::createDescriptorSetLayoutBindings()
{
    VkDescriptorSetLayoutBinding samplerDescriptorSetLayoutBinding{};
	samplerDescriptorSetLayoutBinding.binding = 0;
	samplerDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerDescriptorSetLayoutBinding.descriptorCount = 1;
	samplerDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

	return std::vector<VkDescriptorSetLayoutBinding>{samplerDescriptorSetLayoutBinding};
}

std::map<uint32_t, VkDescriptorImageInfo> Material::createDescriptorImageInfos()
{
    std::map<uint32_t, VkDescriptorImageInfo> imageBindingInfos;
	imageBindingInfos[0] = VkDescriptorImageInfo{
		.sampler = sampler,
		.imageView = imageView,
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	};

    return imageBindingInfos;
}
