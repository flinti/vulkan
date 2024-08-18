#include "VulkanObjectCache.h"
#include "DescriptorSetLayout.h"
#include "VkHelpers.h"
#include "VkHash.h"
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

VulkanObjectCache::VulkanObjectCache(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device)
    : instance(instance),
    physicalDevice(physicalDevice),
    device(device)
{
}

VulkanObjectCache::~VulkanObjectCache()
{
    for (auto &sampler : samplers) {
        vkDestroySampler(device, sampler.second, nullptr);
    }
}


VkSampler VulkanObjectCache::getSampler(const VkSamplerCreateInfo &info)
{
    size_t hash = Utility::hash_value(info);
    auto i = samplers.find(hash);
    if (i != samplers.end()) {
        return i->second;
    }
    VkSampler newSampler = VK_NULL_HANDLE;
    VK_ASSERT(vkCreateSampler(device, &info, nullptr, &newSampler));
    samplers.emplace(hash, newSampler);

    spdlog::info("VulkanObjectCache: created sampler {}", (void*) newSampler);

    return newSampler;
}

DescriptorSetLayout &VulkanObjectCache::getDescriptorSetLayout(
    const std::vector<VkDescriptorSetLayoutBinding> &bindings,
    VkDescriptorSetLayoutCreateFlags flags
)
{
    size_t hash = Utility::hash_value(flags);
    for (auto &binding : bindings) {
        Utility::hash_combine(hash, binding);
    }
    auto i = descriptorSetLayouts.find(hash);
    if (i != descriptorSetLayouts.end()) {
        return *i->second;
    }

    DescriptorSetLayout &layout = *descriptorSetLayouts.emplace(
        hash,
        std::make_unique<DescriptorSetLayout>(device, bindings)
    ).first->second;

    spdlog::info("VulkanObjectCache: created descriptor set layout at {}", (void*) &layout);

    return layout;
}
