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
    uint32_t id,
    Device &device,
    const ShaderResource &vertexShader,
    const ShaderResource &fragmentShader,
    const std::vector<ImageResource> &imageResources,
    const Parameters &parameters,
    std::string name
)
    : id(id),
    device(device),
    vertexShader(vertexShader),
    fragmentShader(fragmentShader),
    images(createImages(imageResources)),
    imageViews(createImageViews()),
    sampler(requestSampler()),
    parameterBuffer(
        device.getAllocator(),
        (void*) &parameters,
        sizeof(parameters),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
    ),
    descriptorSetLayoutBindings(createDescriptorSetLayoutBindings()),
    descriptorImageInfos(createDescriptorImageInfos()),
    descriptorBufferInfos(createDescriptorBufferInfos()),
    name(name)
{
}

Material::Material(Material &&other)
    : id(other.id),
    device(other.device),
    vertexShader(other.vertexShader),
    fragmentShader(other.fragmentShader),
    images(std::move(other.images)),
    imageViews(std::move(other.imageViews)),
    sampler(other.sampler),
    parameterBuffer(std::move(other.parameterBuffer)),
    descriptorSetLayoutBindings(std::move(other.descriptorSetLayoutBindings)),
    descriptorImageInfos(std::move(other.descriptorImageInfos)),
    descriptorBufferInfos(std::move(other.descriptorBufferInfos)),
    name(std::move(other.name))
{
    other.sampler = VK_NULL_HANDLE;
}

Material::~Material()
{
    images.clear();
    for (auto &imageView : imageViews) {
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


const std::map<uint32_t, VkDescriptorBufferInfo> &Material::getDescriptorBufferInfos() const
{
    return descriptorBufferInfos;
}


std::vector<Image> Material::createImages(const std::vector<ImageResource> &imageResources)
{
    spdlog::info("Material {}: creating images", id);

    std::vector<Image> images;
    images.reserve(imageResources.size());

    for (const auto &imageResource : imageResources) {
        images.emplace_back(device, imageResource);
    }

    return images;
}

std::vector<VkImageView> Material::createImageViews()
{
    spdlog::info("Material {}: creating image views", id);

    std::vector<VkImageView> imageViews(images.size(), VK_NULL_HANDLE);

    for (size_t i = 0; i < images.size(); ++i) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = images[i].getImageHandle();
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VK_ASSERT(vkCreateImageView(
            device.getDeviceHandle(),
            &viewInfo,
            nullptr,
            &imageViews[i]
        ));
    }
    
    return imageViews;
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
    spdlog::info("Material {}: creating descriptor set layout bindings", id);

    std::vector<VkDescriptorSetLayoutBinding> bindings(images.size() + 1, VkDescriptorSetLayoutBinding{});

    bindings[0] = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
    };

    for (size_t i = 1; i < bindings.size(); ++i) {
        VkDescriptorSetLayoutBinding &samplerDescriptorSetLayoutBinding = bindings[i];
        samplerDescriptorSetLayoutBinding.binding = i;
        samplerDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerDescriptorSetLayoutBinding.descriptorCount = 1;
        samplerDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }

	return bindings;
}

std::map<uint32_t, VkDescriptorImageInfo> Material::createDescriptorImageInfos()
{
    spdlog::info("Material {}: creating image binding infos", id);

    std::map<uint32_t, VkDescriptorImageInfo> imageBindingInfos;

    for (size_t i = 0; i < images.size(); ++i) {
        imageBindingInfos[i + 1] = VkDescriptorImageInfo{
            .sampler = sampler,
            .imageView = imageViews[i],
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
    }

    return imageBindingInfos;
}


std::map<uint32_t, VkDescriptorBufferInfo> Material::createDescriptorBufferInfos()
{
    return std::map<uint32_t, VkDescriptorBufferInfo>{
        std::make_pair(
            0,
            VkDescriptorBufferInfo{
                .buffer = parameterBuffer.getHandle(),
                .offset = 0,
                .range = VK_WHOLE_SIZE,
            }
        )
    };
}
