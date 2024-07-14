#include "VulkanObjectCache.h"
#include "VkHelpers.h"
#include "VkHash.h"
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
    size_t hash = hash_value(info);
    auto i = samplers.find(hash);
    if (i != samplers.end()) {
        return i->second;
    }
    VkSampler newSampler = VK_NULL_HANDLE;
    VK_ASSERT(vkCreateSampler(device, &info, nullptr, &newSampler));
    samplers.emplace(hash, newSampler);
    return newSampler;
}
